#include "tcp_socket.hpp"

namespace trading{

    int TCPSocket::connect(const std::string &ip, const std::string &iface, int port, bool is_listening){
        SocketConfigs sockConf{ip, iface, port, false, is_listening, true}; //needs_so_timestamp_ = true for FIFO sequencer

        fd_ = createSocket(logger_, sockConf);

        socket_attribute_.sin_addr.s_addr = INADDR_ANY;
        socket_attribute_.sin_port = htons(port);
        socket_attribute_.sin_family = AF_INET; 

        return fd_;
    }

    ///Writing outgoing data to the send_buffer
    void TCPSocket::send(const void* data, size_t len) noexcept{
        memcpy(send_buffer_.data() + send_next_valid_idx_, data, len);
        send_next_valid_idx_ += len;
    }

    bool TCPSocket::sendAndRecv() noexcept {
        char ctrl[CMSG_SPACE(sizeof(struct timeval))];
        auto cmsg = reinterpret_cast<struct cmsghdr *>(&ctrl);

        iovec iov{recv_buffer_.data() + recv_next_valid_idx_, TCPBufferSize - recv_next_valid_idx_}; 
        msghdr msg{&socket_attribute_, sizeof(socket_attribute_), &iov, 1, ctrl, sizeof(ctrl), 0};

        const auto read_size = recvmsg(fd_, &msg, MSG_DONTWAIT);
        if(read_size > 0){
            recv_next_valid_idx_ += read_size;

            Nanos kernel_time = 0;
            timeval time_kernel;
            if(cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP && cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel))){
                memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
                kernel_time = time_kernel.tv_sec * NANOS_TO_SECS + time_kernel.tv_usec * NANOS_TO_MICROS; // convert timestamp to nanoseconds.
            }

            const auto user_time = getCurrentNanos();

            logger_.log("%:% %() % read socket:% len:% utime:% ktime:% diff:%\n", __FILE__, __LINE__, __FUNCTION__,
                          getCurrentTimeStr(&time_str_), fd_, recv_next_valid_idx_, user_time, kernel_time, (user_time - kernel_time));
            recv_callbacks_(this, kernel_time);

        }   
        
        if(send_next_valid_idx_ > 0){
            const auto n = ::send(fd_, send_buffer_.data(), send_next_valid_idx_, MSG_DONTWAIT | MSG_NOSIGNAL); //Global scope send
            logger_.log("%:% %() % send socket:% len:%\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), fd_, n);
        }
        send_next_valid_idx_ = 0;

        return read_size > 0;
    }


}