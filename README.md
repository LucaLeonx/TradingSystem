# TradingSystem

The goal of this project is to learn low-level techniques by designing and implementing a Trading System, both exchange-side and client-side in C++.

All the low-level techniques and optimisations will be used in order to achieve maximum performance and lowest latancy as possible.

Suggestions on further optimization are more than welcomed, everything written in this project is hand-written, **no AI involved**.  

## Exchange
![](docs/images/Exchange.png)

Formed by three main components:
- Market Data Publisher, a component that sends order book updates to all market participants.
- Order Gateway server, sends order updates to the client involved in those updates.
- Matching Engine, encapsulates the logic and the data related to the order book, receives orders from the order gateway and modifies its state accordingly. 
---

### Matching Engine
Is the main component that keeps the state of the order book. It keeps for every asset a list of bid and ask prices, which are the passive orders submitted  by the clients, when there is an aggressive order (either buy or sell) that matches the current available orders, it performs the matching and modifies the state accordingly.

The main complexity is how to handle the data, for the order book we need:
- Asks/bids order prices ordered from lowest-to-highest/highest-to-lowest because of teh nature of an market.
- Fast random access of orders because of possible order cancellation or modification.
- Ordering of passive orders of the same price based on priority (newest orders have less priority than the oldest one).

To achieve this goal we divide the orders into levels based on their prices; every level keeps a double-linked list with all the orders ordered by their priority.
Also the price levels, called MEOrdersAtPrice, are connected through a double-linked list to allow easy navigation and random insertion in constant time; these structures are both for asks and bids.

To allow random lookup in constant time, we also take the references of the orders and the OrdersAtPrice in two different HashMaps. 

![](docs/images/orderBook.png)
 
 ---

### Order Gateway server
Its purpose is to connect with the market participant through TCP (because we don't want to lose data about orders), collect all the order/operation on the exchange and sent to the matching engine.

Through a FIFO sequencer the operations are ordered based on their arrival time, then there is a layer for decoding and encoding the data (note that the format of the internal data and the one received from the client are different).

The information is sent and received to/from the matching engine through a lock-free queue, thanks to it we can share data between two threads without needing synchronisation (the queues are single-producer, single-consumer (SPSC).

![](docs/images/OrderGateway.png)
 ---

### Market Data Publishers

This component goal is send market updates to all the entities registered to our upstream.

It does so by having a multicast socket constantly streaming the state changes on the matching engine. The data are received through a lock-free queue shared with the matching engine, are then encoded in the public format for the market updates and the multicasted.

The chosen socket connection is UDP for performance reasons, in order to handle possible packet loss, there is another sub-component, the **Snapshot Synthetizer**, whose goal is to create a partial state of the order book, so in case of packet loss we can just take the full state of the order book and start getting incremental updates from there.

![](docs/images/marketDataPublisher.png)
 ---

## Market Participant

TODO


