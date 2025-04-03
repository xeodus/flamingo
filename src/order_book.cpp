#include "order_book.hpp";
#include <chrono>
#include <ctime>

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