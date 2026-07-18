#include "client/market_order_book.hpp"

namespace trading::client{
 
    void MarketOrderBook::onMarketUpdate(const trading::exchange::MEMarketUpdate& market_update) noexcept{
        const bool bid_updated = (bids_by_price_ && market_update.side_ == Side::BUY && market_update.price_ >= bids_by_price_->price_);
        const bool ask_updated = (asks_by_price_ && market_update.side_ == Side::SELL && market_update.price_ <= asks_by_price_->price_);

        using namespace trading::exchange;
        switch(market_update.type_){
            case MarketUpdateType::ADD :{
                auto order = order_pool_.allocate(market_update.order_id_, market_update.side_, market_update.price_, market_update.qty_, market_update.priority_, nullptr, nullptr);

                AddOrder(order);
            }
            break;
            case MarketUpdateType::MODIFY :{
                auto order = oid_to_order_[market_update.order_id_];
                order->qty_ = market_update.qty_;
            }
            break;
            case MarketUpdateType::CANCEL :{
                auto order = oid_to_order_[market_update.order_id_];
                removeOrder(order);
            }
            break;
            case MarketUpdateType::TRADE :{
                trade_engine_->onTradeUpdate(market_update, this);
                return;
            }
            break;
            case MarketUpdateType::CLEAR :{
                for(auto &order : oid_to_order_){
                    if(order){
                        order_pool_.deallocate(order);
                    }
                }
                oid_to_order_.fill(nullptr);

                if(bids_by_price_){
                    for(auto bids = bids_by_price_->next_; bids != bids_by_price_; bids = bids_by_price_->next_)    orders_at_price_pool_.deallocate(bids);
                    orders_at_price_pool_.deallocate(bids_by_price_);
                }

                if(asks_by_price_){
                    for(auto asks = asks_by_price_->next_; asks != asks_by_price_; asks = asks_by_price_->next_)    orders_at_price_pool_.deallocate(asks);
                    orders_at_price_pool_.deallocate(asks_by_price_);
                }
            }
            break;
            case MarketUpdateType::SNAPSHOT_START : break;
            case MarketUpdateType::SNAPSHOT_END : break;
            case MarketUpdateType::INVALID : break;
        }

        updateBBO(bid_updated, ask_updated);

        logger_.log("%:% %() % % %", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString(), bbo_.toString());

        trade_engine_->onOrderBookUpdate(market_update.ticker_id_, market_update.price_, market_update.side_, this);
    }

    void MarketOrderBook::updateBBO(bool update_bid, bool update_ask) noexcept{
        if(update_bid){
            if(bids_by_price_){
                bbo_.bid_price_ = bids_by_price_->price_;
                bbo_.bid_qty_ = bids_by_price_->first_order_->qty_;
                for(auto bids = bids_by_price_->first_order_->next_order_; bids != bids_by_price_->first_order_; bids = bids->next_order_)
                    bbo_.bid_qty_ += bids->qty_;
            } else{
                bbo_.bid_qty_ = Qty_INVALID;
                bbo_.bid_price_ = Price_INVALID; 
            }
        }

        if(update_ask){
            if(asks_by_price_){
                bbo_.ask_price_ = asks_by_price_->price_;
                bbo_.ask_qty_ = asks_by_price_->first_order_->qty_;
                for(auto asks = asks_by_price_->first_order_->next_order_; asks != asks_by_price_->first_order_; asks = asks->next_order_)
                    bbo_.ask_qty_ += asks->qty_;
            } else{
                bbo_.ask_qty_ = Qty_INVALID;
                bbo_.ask_price_ = Price_INVALID; 
            }
        }
    }

    void MarketOrderBook::AddOrderAtPrice(MarketOrdersAtPrice* new_orderAtPrice) noexcept{
        oid_to_price_level_.at(priceToIndex(new_orderAtPrice->price_)) = new_orderAtPrice;

        const auto orderPriceListHead = (new_orderAtPrice->side_ == Side::BUY) ? bids_by_price_ : asks_by_price_;
        if(orderPriceListHead == nullptr){
            ((new_orderAtPrice->side_ == Side::BUY) ? bids_by_price_ : asks_by_price_) = new_orderAtPrice;
            new_orderAtPrice->next_ = new_orderAtPrice->prev_ = new_orderAtPrice;
        } else { //The OrderAtPrice list exists and we need to find where to place our new OrderAtPrice 
            MarketOrdersAtPrice* curr = orderPriceListHead;
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

    void MarketOrderBook::AddOrder(MarketOrder* order) noexcept{
        auto orderAtPrice = getOrdersAtPrice(order->price_);
        
        if(!orderAtPrice){
            orderAtPrice = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
            AddOrderAtPrice(orderAtPrice);
        } else {
            auto levelHead = orderAtPrice->first_order_; 

            levelHead->prev_order_->next_order_ = order;
            order->next_order_ = levelHead;
            order->prev_order_ = levelHead->prev_order_;
            levelHead->prev_order_ = order;
        }

        oid_to_order_.at(order->order_id_) = order;
    }

    void MarketOrderBook::removeOrder(MarketOrder* order) noexcept{
        auto orderLevel = getOrdersAtPrice(order->price_);

        if(orderLevel->first_order_ == order->prev_order_){
            removeOrderAtPrice(orderLevel);
        } else { 
            order->prev_order_->next_order_ = order->next_order_;
            order->next_order_->prev_order_ = order->prev_order_;
            if(order == orderLevel->first_order_){
                orderLevel->first_order_ = order->next_order_; 
            }

            order->next_order_ = order->prev_order_ = nullptr;
        }

        oid_to_order_[order->order_id_] = nullptr;
        order_pool_.deallocate(order);
    }

    void MarketOrderBook::removeOrderAtPrice(MarketOrdersAtPrice* orderAtPrice) noexcept{
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
        oid_to_price_level_[priceToIndex(orderAtPrice->price_)] = nullptr;
        orders_at_price_pool_.deallocate(orderAtPrice);
    }
    
}