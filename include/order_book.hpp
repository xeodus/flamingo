#pragma once 

#include "utils.hpp"
#include "order.hpp"
#include "ob_levelinfo.hpp"
#include "order_modify.hpp"
#include "trade.hpp"
#include <map>
#include <thread>

class OrderBook {

private:
    struct OrderEntry {
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;
    };

    struct LevelData {
        Quantity quantity_{};
        Quantity count_{};

        enum class Actions {
            Add,
            Remove,
            Match
        };
    };

    std::unordered_map<Price, LevelData> data_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::greater<Price>> asks_;
    std::unordered_map<OrderID, OrderEntry> orders_;
    mutable std::mutex orders_mutex;
    std::thread orders_prune_thread;
    std::condition_variable shutdown_conditional_variables;
    std::atomic<bool> shutdown{false};

    void prune_good_for_day();
    void cancel_orders(OrderIDs order_ids);
    void cancel_internal_order(OrderID order_id);
    void on_order_canceled(OrderPointer order);
    void on_order_add(OrderPointer order);
    void on_order_matched(Price price, Quantity quantity, bool is_fully_filled);
    void update_level_data(Price price, Quantity quantity, LevelData::Actions actions);
    bool can_fully_fill(Price price, Quantity quantity, Side side) const;
    bool can_match(Price price, Side side) const;
    Trades match_orders();
    
public:
    OrderBook();
    OrderBook(const OrderBook&) = delete;
    void operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    void operator=(const OrderBook&&) = delete;
    ~OrderBook();

    Trades add_orders(OrderPointer order);
    void cancel_order(OrderID order_id);
    Trades modify_order(OrderModify order);
    std::size_t Size() const;
    OrderBookLevelInfo get_order_infos() const;
};