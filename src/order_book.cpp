#include "order_book.hpp";
#include <chrono>
#include <ctime>
#include <numeric>

void OrderBook::prune_good_for_day() {
    using namespace std::chrono;
    const auto end = hours(16);

    while (true) {
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);
        std::tm now_parts;
        localtime_r(&now_c, &now_parts);

        if (now_parts.tm_hour >= end.count()) {
            now_parts.tm_mday += 1;
        }

        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        auto next = system_clock::from_time_t(mktime(&now_parts));
        auto till = next - now + milliseconds(100);

        {
            std::unique_lock orders_lock{orders_mutex};

            if (shutdown.load(std::memory_order_acquire) || shutdown_conditional_variables.wait_for(orders_lock, till) == std::cv_status::no_timeout) {
                return;
            }
        }

        OrderIDs order_ids;

        {
            std::scoped_lock orders_lock{orders_mutex};

            for (const auto& [_, entry]: orders_) {
                const auto& [order, _] = entry;

                if (order->get_order_type() != OrderType::GoodForDay) {
                    continue;
                }

                order_ids.push_back(order->get_order_id());
            }
        }
        cancel_orders(order_ids);
    }
}

void OrderBook::cancel_orders(OrderIDs order_ids) {
    std::scoped_lock orders_lock{orders_mutex};

    for (const auto& order_id: order_ids) {
        cancel_internal_order(order_id);
    }
}

void OrderBook::cancel_internal_order(OrderID order_id) {

    if (!orders_.contains(order_id)) {
        return;
    }

    const auto& [order, iterator] = orders_.at(order_id);
    orders_.erase(order_id);

    on_order_canceled(order);
}

void OrderBook::on_order_canceled(OrderPointer order) {
    update_level_data(order->get_price(), order->get_remaining_quantity(), LevelData::Actions::Remove);
}

void OrderBook::on_order_add(OrderPointer order) {
    update_level_data(order->get_price(), order->get_initial_quantity(), LevelData::Actions::Add);
}

void OrderBook::on_order_matched(Price price, Quantity quantity, bool is_fully_filled) {
    update_level_data(price, quantity, is_fully_filled ? LevelData::Actions::Remove : LevelData::Actions::Match);
}

void OrderBook::update_level_data(Price price, Quantity quantity, LevelData::Actions actions) {
    auto& data = data_[price];
    data.count_ += actions == LevelData::Actions::Remove ? -1 : actions == LevelData::Actions::Add ? 1 : 0;

    if (actions == LevelData::Actions::Remove || actions == LevelData::Actions::Match) {
        data.quantity_ -= quantity;
    }
    else {
        data.quantity_ += quantity; 
    }

    if (data.count_ == 0) {
        data_.erase(price);
    }
}

bool OrderBook::can_fully_fill(Price price, Quantity quantity, Side side) const {

    if (!can_match(price, side)) {
        return false;
    }

    std::optional<Price> threshold;

    if (side == Side::Buy) {
        const auto& [ask_price, _] = *asks_.begin();
        threshold = ask_price;
    }
    else {
        const auto& [bid_price, _] = *bids_.begin();
        threshold = bid_price;
    }

    for (const auto& [level_price, level_data]: data_) {

        if (threshold.has_value() &&
        (side == Side::Buy && threshold.value() > level_price) ||
        (side == Side::Sell && threshold.value() < level_price)) {
            continue;
        }

        if ((side == Side::Buy && level_price > price) ||
        (side == Side::Sell && level_price < price)) {
            continue;
        }

        if (quantity <= level_data.quantity_) {
            return true;
        }

        quantity -= level_data.quantity_;
    }
    return false;
}

bool OrderBook::can_match(Price price, Side side) const {
    
    if (side == Side::Buy) {
        if (asks_.empty()) {
            return false;
        }

        const auto& [best_ask, _] = *asks_.begin();
        return price >= best_ask;
    }
    else {
        if (bids_.empty()) {
            return false;
        }

        const auto& [best_bid, _] = *bids_.begin();
        return price <= best_bid;
    }
}

Trades OrderBook::match_orders() {
    Trades trades;
    trades.reserve(orders_.size());

    while (true) {
        if (bids_.empty() || asks_.empty()) {
            break;
        }

        auto& [bid_price, bids] = *bids_.begin();
        auto& [ask_price, asks] = *asks_.begin();

        if (bid_price < ask_price) {
            break;
        }

        while (!bids.empty() && !asks.empty()) {
            auto bid = bids.front();
            auto ask = asks.front();

            Quantity quantity = std::min(bid->get_remaining_quantity(), ask->get_initial_quantity());

            bid->fill(quantity);
            ask->fill(quantity);

            if (bid->is_filled()) {
                bids.pop_front();
                orders_.erase(bid->get_order_id());
            }

            if (ask->is_filled()) {
                asks.pop_front();
                orders_.erase(ask->get_order_id());
            }

            trades.push_back(Trade {
                TradeInfo { bid->get_order_id(), bid->get_price(), quantity },
                TradeInfo{ ask->get_order_id(), ask->get_price(), quantity }
            });
        }

        if (bids.empty()) {
            bids_.erase(bid_price);
            data_.erase(bid_price);
        }

        if (asks.empty()) {
            asks_.erase(ask_price);
            orders_.erase(ask_price);
        }

        if (!bids.empty()) {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if (order->get_order_type() == OrderType::FillAndKill) {
                cancel_order(order->get_order_id());
            }
        }

        if (!asks.empty()) {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
            if (order->get_order_type() == OrderType::FillAndKill) {
                cancel_order(order->get_order_id());
            }
        }
    }
    return trades;
}

OrderBook::OrderBook(): orders_prune_thread{ [this] { prune_good_for_day(); }} {}

OrderBook::~OrderBook() {
    shutdown.store(true, std::memory_order_release);
    shutdown_conditional_variables.notify_one();
    orders_prune_thread.join();
}

Trades OrderBook::add_orders(OrderPointer order) {
    std::scoped_lock orders_lock{orders_mutex};

    if (orders_.contains(order->get_order_id())) {
        return {};
    }

    if (order->get_order_type() == OrderType::Market) {
        if (order->get_side() == Side::Buy && !asks_.empty()) {
            const auto& [work_ask, _] = *asks_.rbegin();
            order->to_good_till_cancel(work_ask);
        }
        else if (order->get_side() == Side::Sell && !bids_.empty()) {
            const auto& [work_bid, _] = *bids_.rbegin();
            order->to_good_till_cancel(work_bid);
        }
        else {
            return {};
        }
    }

    if (order->get_order_type() == OrderType::FillAndKill && !can_match(order->get_price(), order->get_side())) {
        return {};
    }

    if (order->get_order_type() == OrderType::FillOrKill && !can_match(order->get_price(), order->get_side())) {
        return {};
    }

    OrderPointers::iterator iterator;

    if (order->get_side() == Side::Buy) {
        auto& orders = asks_[order->get_price()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    else {
        auto& orders = bids_[order->get_price()];
        orders.push_back(order);
        iterator = std::prev(orders.end());
    }

    orders_.insert({ order->get_order_id(), OrderEntry{ order, iterator } });
    on_order_add(order);
    
    return match_orders();
}

void OrderBook::cancel_order(OrderID order_id) {
    std::scoped_lock orders_lock{orders_mutex};

    cancel_internal_order(order_id);
}

std::size_t OrderBook::Size() const {
    std::scoped_lock orders_lock{orders_mutex};
    return orders_.size();
}

Trades OrderBook::modify_order(OrderModify order) {
    OrderType order_type;

    {
        std::scoped_lock orders_lock{orders_mutex};
        if (!orders_.contains(order.get_order_id())) {
            return {};
        }
        const auto& [exsiting_order, _] = orders_.at(order.get_order_id());
        order_type = exsiting_order->get_order_type();
    }

    cancel_order(order.get_order_id());
    return add_orders(order.to_order_pointer(order_type));
}

OrderBookLevelInfo OrderBook::get_order_infos() const {
    
    LevelInfos bid_infos, ask_infos;
    bid_infos.reserve(orders_.size());
    ask_infos.reserve(orders_.size());
    
    auto create_level_infos = [] (Price price, const OrderPointers& orders) {
        return LevelInfo {
            price, std::accumulate(orders.begin(), orders.end(), (Quantity)0, [] (Quantity running_sum, const OrderPointer& order)
            { return running_sum + order->get_remaining_quantity(); })
        };
    };

    for (const auto& [price, orders]: bids_) {
        bid_infos.push_back(create_level_infos(price, orders));
    }

    for (const auto& [price, orders]: asks_) {
        ask_infos.push_back(create_level_infos(price, orders));
    }

    return OrderBookLevelInfo { bid_infos, ask_infos};
} 