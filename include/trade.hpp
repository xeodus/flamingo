#pragma once

#include "trade_info.hpp"
#include <vector>

class Trade {

public:
    Trade(const TradeInfo& bid_trade, const TradeInfo& ask_trade): bid_trade_{bid_trade}, ask_trade_{ask_trade} {};

    const TradeInfo& get_bid_trade() { return bid_trade_; }
    const TradeInfo& get_ask_trade() { return ask_trade_; }
    
private:
    TradeInfo bid_trade_;
    TradeInfo ask_trade_;

};

using Trades = std::vector<Trade>;