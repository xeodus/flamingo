#pragma once 

#include "level_info.hpp"

class OrderBookLevelInfo {

public:
    OrderBookLevelInfo(const LevelInfo& bids, const LevelInfo& asks): bids_{bids}, asks_{asks} {}

    const LevelInfo& get_bid_info() const { return bids_; }
    const LevelInfo& get_ask_info() const { return asks_; }

private:
    LevelInfo bids_;
    LevelInfo asks_;
};