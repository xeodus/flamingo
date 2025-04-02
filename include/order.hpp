#pragma once

#include "order_type.hpp"
#include "side.hpp"
#include "utils.hpp"
#include "const.hpp"
#include <format>
#include <list>

class Order {

public:
    Order(OrderType order_type, OrderID order_id, Price price, Side side, Quantity quantity): order_type_{order_type}, order_id_{order_id}, price_{price}, side_{side}, initial_quantity{quantity}, remaining_quantity{quantity} {}
    // Order(OrderID order_id, Side side, Quantity quantity): Order(OrderType::Market, order_id, side, Constants::InvalidPrice, quantity) {}

    OrderType get_order_type() const { return order_type_; }
    OrderID get_order_id() const { return order_id_; }
    Side get_side() const { return side_; }
    Price get_price() const { return price_; }
    Quantity get_initial_quantity() const { return initial_quantity; }
    Quantity get_remaining_quantity() const { return remaining_quantity; }
    Quantity get_filled_quantity() const { return get_initial_quantity() - get_remaining_quantity(); }
    bool is_filled() const { return get_remaining_quantity() == 0; }

    void fill(Quantity quantity) {

        if (quantity > get_remaining_quantity()) {
            throw std::logic_error(std::format("Order {} cannot be filled as order size exceeded the remaining size.", get_order_id()));
        }

        remaining_quantity -= quantity;
    }

    void to_good_till_cancel(Price price) {

        if (get_order_type() != OrderType::Market) {
            throw std::logic_error(std::format("Order {} cannot re-adjust itself, only market orders can.", get_order_id()));
        }

        price_ = price;
        order_type_ = OrderType::GoodTillCancel;
    }

private:
    OrderType order_type_;
    OrderID order_id_;
    Side side_;
    Price price_;
    Quantity initial_quantity;
    Quantity remaining_quantity;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;