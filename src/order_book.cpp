#include "exchange/order_book.hpp"
#include "exchange/matching_engine.hpp"

namespace trading::exchange {

    MEOrderBook::MEOrderBook(TickerId tickerId, Logger& logger, MatchingEngine& matching_engine)    
            : ticker_(tickerId), matching_engine_(matching_engine), orders_at_price_pool_(ME_MAX_PRICE_LEVELS), orders_pool_(ME_MAX_ORDER_IDS), logger_(logger) {

    }

    MEOrderBook::~MEOrderBook(){
        logger_.log("%:% %() %\tDestroying the Order Book for the ticker: %, SELF DESTRUCTION IN  3   2   1...\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), tickerIdToString(ticker_));

        bids_by_price_ = nullptr;
        asks_by_price_ = nullptr;

        for(auto& order_hashmap : cid_oid_to_order_){
            order_hashmap.fill(nullptr);
        }
    }

    /// Add request to the order book
    void MEOrderBook::add(ClientId clientId, OrderId client_orderId, Side side, Price price, Qty qty) noexcept{
        const OrderId new_market_order_id = generateNextOrderId();

        matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::ACCEPTED, clientId, ticker_, client_orderId, new_market_order_id, side, price, 0, qty});

        const auto qty_left = checkForMatch(clientId, client_orderId, side, price, qty, new_market_order_id);//TODO: implement

        if(qty_left > 0){
            const auto priority = getPriority(price);

            auto orderObj = orders_pool_.allocate(ticker_, clientId, client_orderId, new_market_order_id, side, price, qty_left, priority, nullptr, nullptr);

            AddOrderToPriceLevel(orderObj);

            matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::ADD, new_market_order_id, ticker_, side, price, qty, priority});
        }

    }

    ///Add Order to its OrderAtPrice level, if its related OrderAtPrice doesn't exists it allocate one through the OrderAtPrice memory pool 
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

    /// Find the right spot of the orderAtPrice object into the OrderAtPrice Doubly-linked list, based on the price and on the side
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

    void MEOrderBook::cancel(ClientId clientid, OrderId orderId) noexcept{
        if(clientid >= cid_oid_to_order_.size() || orderId >= cid_oid_to_order_.at(clientid).size() || cid_oid_to_order_.at(clientid).at(orderId) == nullptr){
            matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::CANCEL_REJECTED, clientid, ticker_, orderId,
                                                                    OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID});
            return;
        }

        auto order = cid_oid_to_order_.at(clientid).at(orderId);

        removeOrder(order);

        matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::CANCELLED, clientid, ticker_, orderId, order->market_order_id_, order->side_, order->price_, Qty_INVALID, order->qty_});
        matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::CANCEL, order->market_order_id_, ticker_, order->side_, order->price_, order->qty_, order->priority_});
    }

    ///Find the order in the OrderAtPrice, if the level is contains only this order deallocate the OrderAtPrice level from its pool, update the list, then deallocate order from its pool
    void MEOrderBook::removeOrder(MEOrder* order) noexcept{
        auto orderLevel = getOrderAtPrice(order->price_);

        if(orderLevel->orders_head_ == order->prev_order_){
            removeOrderAtPrice(orderLevel);
        } else { 
            order->prev_order_->next_order_ = order->next_order_;
            order->next_order_->prev_order_ = order->prev_order_;
            if(order == orderLevel->orders_head_){
                orderLevel->orders_head_ = order->next_order_; 
            }

            order->next_order_ = order->prev_order_ = nullptr;
        }

        cid_oid_to_order_[order->client_id_][order->client_id_] = nullptr;
        orders_pool_.deallocate(order);
    }

    void MEOrderBook::removeOrderAtPrice(MEOrdersAtPrice* orderAtPrice) noexcept{
        auto& ordersLevelsHead = (orderAtPrice->side_ == Side::BUY) ? bids_by_price_ : asks_by_price_;
        
        if(ordersLevelsHead == orderAtPrice){
            ordersLevelsHead = nullptr;
        } else {
            //Update the linked-list by removing the input object
            orderAtPrice->prev_->next_ = orderAtPrice->next_;
            orderAtPrice->next_->prev_ = orderAtPrice->prev_;
            if(ordersLevelsHead == orderAtPrice){ //Check the head of the linked list
                ordersLevelsHead = orderAtPrice->next_;
            }
            //Deallocate all of its orders
            orderAtPrice->prev_ = orderAtPrice->next_ = nullptr;
        }
        price_to_orders_at_price_[orderAtPrice->price_] = nullptr;
        orders_at_price_pool_.deallocate(orderAtPrice);
    }

    Qty MEOrderBook::checkForMatch(ClientId clientId, OrderId client_orderId, Side side, Price price, Qty qty, OrderId market_order_id){
        Qty left_over = qty;

        if(side == Side::BUY){
            while(left_over > 0 && asks_by_price_){
                if(price < asks_by_price_->orders_head_->price_){
                    break;
                }
                match(clientId, client_orderId, market_order_id, side, asks_by_price_->orders_head_, left_over);
            }

        } else if(side == Side::SELL){
            while (left_over > 0 && bids_by_price_){
                if(price > bids_by_price_->orders_head_->price_){
                    break;
                }
                match(clientId, client_orderId, market_order_id, side, bids_by_price_->orders_head_, left_over);
            }
        }

        return left_over;
    }

    ///Given a passive order and an aggressive order that match handle the matching
    void MEOrderBook::match(ClientId clientId, OrderId client_orderId, OrderId market_orderId, Side side, MEOrder* order, Qty& left) noexcept{
        const auto order_qty = order->qty_;
        const auto filled_qty = std::min(order_qty, left);

        left -= filled_qty;
        order->qty_ -= filled_qty;

        //answer to the client of the aggressive order 
        matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::FILLED, clientId, ticker_, client_orderId, 
                                                            market_orderId, side, order->price_, filled_qty, left});

        //answer to the client of the passive order 
        matching_engine_.sendClientResponse(MEClientResponse{ClientResponseType::FILLED, order->client_id_, ticker_, order->client_order_id_, 
                                                            order->market_order_id_, order->side_, order->price_, filled_qty, order->qty_});

        matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::TRADE, OrderId_INVALID, ticker_, side, order->price_, filled_qty, Priority_INVALID});            


        if(order_qty == 0){
            matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::CANCEL, order->market_order_id_, ticker_, order->side_, order->price_, order->qty_, order->priority_});            

            removeOrder(order);
        } else {
            matching_engine_.sendMarketUpdate(MEMarketUpdate{MarketUpdateType::MODIFY, order->market_order_id_, ticker_, order->side_, order->price_, order->qty_, order->priority_});            
        }
                
    }

}