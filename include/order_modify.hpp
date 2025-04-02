#pragma once

#include "order.hpp"

class OrderModify {

public:
    OrderModify(OrderID order_id, Price price, Side side, Quantity quantity): order_id_{order_id}, price_{price}, side_{side}, quantity_{quantity} {}

    OrderID get_order_id() const { return order_id_; }
    Side get_side() const { return side_; }
    Price get_price() const { return price_; }
    Quantity get_quantity() const { return quantity_; }

    OrderPointer to_order_pointer(OrderType type) {
        return std::make_shared<Order> (type, get_order_id(), get_side(), get_price(), get_quantity());
    }

private:
    OrderID order_id_;
    Side side_;
    Price price_;
    Quantity quantity_;
};