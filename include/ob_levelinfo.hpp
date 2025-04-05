#pragma once 

#include "level_info.hpp"

class OrderBookLevelInfo {

public:
    OrderBookLevelInfo(const LevelInfos& bids, const LevelInfos& asks): bids_{bids}, asks_{asks} {}

    const LevelInfos& get_bid_info() const { return bids_; }
    const LevelInfos& get_ask_info() const { return asks_; }

private:
    LevelInfos bids_;
    LevelInfos asks_;
};