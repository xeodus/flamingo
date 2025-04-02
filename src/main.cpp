#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <list>
#include <deque>
#include <memory>

enum class OrderType {
    GoodTillCancel,
    FillnKill,
};

enum class Side {
    Buy,
    Sell,
};

// Defined aliases types

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

// As an order book consists of levels, we will define structs for that

struct LevelInfo {
    Price price_;
    Quantity quantity_;
};

using LevelInfo = std::vector<LevelInfo>;

// Encapsulating LevelInfo as order book can have two sides
// and side consists of several levels 

class OrderBookLevelInfo {
    
public:
    OrderBookLevelInfo(const LevelInfo& bids, const LevelInfo& asks): bids_{bids}, asks_{asks} {}
    const LevelInfo& get_bid_info() const { return bids_; }
    const LevelInfo& get_asks_info() const { return asks_; }

private:
    const LevelInfo& bids_;
    const LevelInfo& asks_;

};

class Order {

public:
    Order(OrderType order_type, OrderId order_id, Price price, Side side, Quantity quantity): order_type_{order_type}, order_id_{order_id}, price_{price}, side_{side}, initial_quantity{quantity}, remaining_quantity{quantity} {}
    OrderType get_order_type() const { return order_type_; }
    OrderId get_order_id() const { return order_id_; }
    Price get_price() const { return price_; }
    Side get_side() const { return side_; }
    Quantity get_initial_quantity() const { return initial_quantity; }
    Quantity get_remaining_quantity() const { return remaining_quantity; }
    Quantity get_filled_quantiy() const { return get_initial_quantity() - get_remaining_quantity(); }

    void fill(Quantity quantity) {

        if (quantity > get_remaining_quantity()) {
            throw std::logic_error(std::format("Order {} cannot be filled as order size exceeded the remaining size.", get_order_id()));
        }

        remaining_quantity -= quantity;

    }

    bool is_filled() {
        while (remaining_quantity == 0) {
            return true;
        }
    }

private:
    OrderType order_type_;
    OrderId order_id_;
    Price price_;
    Side side_;
    Quantity initial_quantity;
    Quantity remaining_quantity;
};

// As the orders can be found in the dictionary as well as a bid or asks based directionary,
// so we will use shared ptr here
using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify {

public:
    OrderModify(OrderId order_id, Side side, Price price, Quantity quantity): order_id_{order_id}, side_{side}, price_{price}, quantity_{quantity} {}
    OrderId get_order_id() const { return order_id_; }
    Side get_side() const { return side_; }
    Price get_price() const { return price_; }
    Quantity get_quantity() const { return quantity_; }

    // Actually this part is a bit redundant, here we are using order pointer to access
    // existing orders and modify them as new orders
    OrderPointer get_order_pointer(OrderType order_type) const {
        return std::make_shared<Order> (order_type, get_order_id(), get_price(), get_side(), get_quantity());
    }

private:
    OrderId order_id_;
    Side side_;
    Price price_;
    Quantity quantity_;

};

struct TradeInfo {
    Price price_;
    OrderId order_id_;
    Quantity quantity_;
};

class Trade {
    
public:
    Trade(const TradeInfo& bid_trade, const TradeInfo& ask_trade): bid_trade_{bid_trade}, ask_trade_{ask_trade} {}
    const TradeInfo& get_bid_trade() const { return bid_trade_; }
    const TradeInfo& get_ask_trade() const { return ask_trade_; }

private:
    const TradeInfo& bid_trade_;
    const TradeInfo& ask_trade_;

};

using Trades = std::vector<Trade>;

class OrderBook {

private:
    struct OrderEntry {
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool can_match(Side side, Price price) {

        if (side == Side::Buy) {

            if (asks_.empty()) {
                return false;
            }

            const auto& [best_asks, _] = *asks_.begin();
            return price >= best_asks;
        }
        else {
            if (bids_.empty()) {
                return false;
            }

            const auto& [best_bids, _] = *bids_.begin();
            return price <= best_bids;
        }
    }

    Trades match_orders() {
        Trades trade;
        trade.reserve(orders_.size());

        while (true) {
            
            if (bids_.empty() || asks_.empty()) {
                break;
            }

            auto& [bid_price, bids] = *bids_.begin();
            auto& [ask_price, asks] = *asks_.begin();

            if (bid_price < ask_price) {
                break;
            }

            while (bids.size() && asks.size()) {
                auto& bid = bids.front();
                auto& ask = asks.front();

                Quantity quantity = std::min(bid->get_remaining_quantity(), ask->get_remaining_quantity());
                bid->fill(quantity);
                ask->fill(quantity);

                if (bid->is_filled()) {
                    bids.pop_front();
                    orders_.erase(bid->get_order_id());
                }
                else if (ask->is_filled()) {
                    asks.pop_front();
                    orders_.erase(ask->get_order_id());
                }
                else if (bids.empty()) {
                    bids_.erase(bid_price);
                }
                else if (asks.empty()) {
                    asks_.erase(ask_price);
                }

                trade.emplace_back(
                    TradeInfo {bid_price, bid->get_order_id(), quantity},
                    TradeInfo {ask_price, ask->get_order_id(), quantity}
                );

            }
        }
        // As some of the orders might not be filled
        if (!bids_.empty()) {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();

            if (order->get_order_type() == OrderType::FillnKill) {
                cancel_order(order->get_order_id());
            }
        }

        if (!asks_.empty()) {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();

            if (order->get_order_type() == OrderType::GoodTillCancel) {
                cancel_order(order->get_order_id());
            }
        }
        return trade;
    }

public:
    Trades add_order(OrderPointer order) {

        if (orders_.contains(order->get_order_id())) {
            return {};
        }

        if (order->get_order_type() == OrderType::FillnKill && !can_match(order->get_side(), order->get_price())) {
            return {};
        }

        OrderPointers::iterator iterator;

        if (order->get_side() == Side::Buy) {
            auto& orders = bids_[order->get_price()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        else {
            auto& orders = asks_[order->get_price()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        
        orders_.insert({ order->get_order_id(), OrderEntry { order, iterator } });

        return match_orders();
    }

    void cancel_order(OrderId order_id) {
        
        if (!orders_.contains(order_id)) {
            return;
        }

        const auto& [order, iterator] = orders_.at(order_id);
        orders_.erase(order_id);

        if (order->get_side() == Side::Sell) {
            auto price = order->get_price();
            auto& orders = orders_.at(price);
            orders.erase(iterator);

            if (orders_.empty()) {
                asks_.erase(price);
            }
        }
        else {
            auto price = order->get_price();
            auto& orders = orders_.at(price);
            orders.erase(iterator);
            
            if (orders_.empty()) {
                bids_.erase(price);
            }
        }
    }

    Trades match_order(OrderModify order) {

        if (!orders_.contains(order.get_order_id())) {
            return {};
        }

        const auto& [existing_orders, _] = orders_.at(order.get_order_id());
        cancel_order(order.get_order_id());
        return add_order(order.get_order_pointer(existing_orders->get_order_type()));
    }

    std::size_t size() const { return orders_.size(); }

    OrderBookLevelInfo get_order_infos() const {
        LevelInfo bid_info, ask_info;
        bid_info.reserve(orders_.size());
        ask_info.reserve(orders_.size());

        
        
    }

};
