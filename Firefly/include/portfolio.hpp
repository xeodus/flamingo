#pragma once

#include "option.hpp"
#include <string>
#include <map>

struct Position {
    std::string instrument_Id;
    double quantity_;
    double ave_price_;
    double current_price_;
    double pnl_;
};

class Portfolio {

public:
    Portfolio(double initial_cash, std::string base_currency);
    ~Portfolio();

    void add_stock_position(std::string symbol, double price, double quantity);
    void add_option_position(std::shared_ptr<Option> option, double price, double quantity);
    void update_prices(std::map<std::string, double> prices);
    double calculate_value() const;
    double calculate_portfolio_delta() const;
    double calculate_portfolio_gamma() const;
    double calculate_portfolio_theta() const;
    double calculate_portfolio_vega() const;
    double calculate_portfolio_rho() const;

    const std::map<std::string, Position>& get_positions() const;
    const std::vector<std::shared_ptr<Option>>& get_options() const;
    double get_cash() const;

private:
    std::map<std::string, Position> positions_;
    std::vector<std::shared_ptr<Option>> options_;
    double cash_;
    std::string base_currency_;
};