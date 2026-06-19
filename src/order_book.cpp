#include "exchange/order_book.hpp"
#include "exchange/matching_engine.hpp"

namespace trading::exchange {

    MEOrderBook::MEOrderBook(TickerId tickerId, Logger& logger, MatchingEngine& matching_engine)    
            : ticker_(tickerId), matching_engine_(matching_engine), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), orders_pool_(ME_MAX_ORDER_IDS), logger_(logger) {

    }

    MEOrderBook::~MEOrderBook(){
        logger_.log("%:% %() %\tDestroying the Order Book for the ticker:, SELF DESTRUCTION IN  3   2   1...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), ticker_);

        bids_by_price_ = nullptr;
        asks_by_price_ = nullptr;

        for(auto& order_hashmap : cid_oid_to_order_){
            order_hashmap.fill(nullptr);
        }
    }

    void MEOrderBook::add(ClientId clientId, OrderId client_orderId, TickerId tickerId, Side side, Price price, Qty qty) noexcept{
        const OrderId new_market_order_id = generateNextOrderId();

        matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::ACCEPTED, clientId, tickerId, client_orderId, new_market_order_id, side, price, 0, qty});

        const auto qty_left = checkForMatch(clientId, client_orderId, tickerId, side, price, qty, new_market_order_id);//TODO: implement

        if(qty_left > 0){
            const auto priority = getPriority(price);

            auto orderObj = orders_pool_.allocate(tickerId, clientId, client_orderId, new_market_order_id, side, price, qty_left, priority, nullptr, nullptr);

            AddOrderToPriceLevel(orderObj);

            matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::ADD, new_market_order_id, tickerId, side, price, priority});
        }

    }

    void MEOrderBook::AddOrderToPriceLevel(MEOrder* order) noexcept{
        auto orderAtPrice = getOrderAtPrice(order->price_);
        
        if(!orderAtPrice){
            orderAtPrice = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
            AddOrderAtPrice(orderAtPrice);
        } else {
            auto levelHead = orderAtPrice->orders_head_; 

            levelHead->prev_order_->next_order_ = order;
            order->next_order_ = levelHead;
            order->prev_order_ = levelHead->prev_order_;
            levelHead->prev_order_ = order;
        }

        cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = order;
    }

    void MEOrderBook::AddOrderAtPrice(MEOrdersAtPrice* new_orderAtPrice) noexcept{
        price_to_orders_at_price_.at(priceToIndex(new_orderAtPrice->price_)) = new_orderAtPrice;

        const auto orderPriceListHead = (new_orderAtPrice->side_ == Side::BUY) ? bids_by_price_ : asks_by_price_;
        if(orderPriceListHead == nullptr){
            ((new_orderAtPrice->side_ == Side::BUY) ? bids_by_price_ : asks_by_price_) = new_orderAtPrice;
            new_orderAtPrice->next_ = new_orderAtPrice->prev_ = new_orderAtPrice;
        } else { //The OrderAtPrice list exists and we need to find where to place our new OrderAtPrice 
            MEOrdersAtPrice* curr = orderPriceListHead;
            bool move_cond = (new_orderAtPrice->side_ == Side::BUY && new_orderAtPrice->price_ < curr->price_) 
                              || (new_orderAtPrice->side_ == Side::SELL && new_orderAtPrice->price_ > curr->price_);

            if(move_cond){ //check for move the curr to the second element if exists
                curr = curr->next_;
                move_cond = (new_orderAtPrice->side_ == Side::BUY && new_orderAtPrice->price_ < curr->price_) 
                                  || (new_orderAtPrice->side_ == Side::SELL && new_orderAtPrice->price_ > curr->price_);
            }

            while(move_cond && curr != orderPriceListHead){
                move_cond = (new_orderAtPrice->side_ == Side::BUY && new_orderAtPrice->price_ < curr->price_) 
                                  || (new_orderAtPrice->side_ == Side::SELL && new_orderAtPrice->price_ > curr->price_);
                
                if(move_cond)  curr = curr->next_;
            }

            // curr is either the head (wrapped around) OR the node we want to insert before.
            // In either case, we want to insert after curr->prev_.
            curr = curr->prev_;
            new_orderAtPrice->next_ = curr->next_;
            new_orderAtPrice->prev_ = curr;
            curr->next_->prev_ = new_orderAtPrice;
            curr->next_ = new_orderAtPrice;

                // Check if new price is better than head, and update head if needed
            if ((new_orderAtPrice->side_ == Side::BUY && new_orderAtPrice->price_ > orderPriceListHead->price_) ||
                (new_orderAtPrice->side_ == Side::SELL && new_orderAtPrice->price_ < orderPriceListHead->price_)) 
            {
                (new_orderAtPrice->side_ == Side::BUY ? bids_by_price_ : asks_by_price_) = new_orderAtPrice;
            }
            
            
        }
        
    }

    void MEOrderBook::cancel([[maybe_unused]] ClientId clientid,[[maybe_unused]]  OrderId orderId,[[maybe_unused]]  TickerId tickerId) noexcept{
        
    }


    Qty MEOrderBook::checkForMatch([[maybe_unused]] ClientId clientId,[[maybe_unused]] OrderId client_orderId,[[maybe_unused]] TickerId tickerId,[[maybe_unused]] Side side,[[maybe_unused]] Price price,[[maybe_unused]] Qty qty,[[maybe_unused]] OrderId market_order_id){
        return 0;
    }

}