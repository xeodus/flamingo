#pragma once 

#include <string>
#include <map>

struct Quote {
    double bid;
    double ask;
    double last;
    double volume;
    double timestamp;
};

struct VolatilityTerm {
    double expiry;
    double volatility;
};

class Market {

public:
    const Quote& get_quote(const std::string& symbol) const;
    void update_quote(const std::string& symbol, const Quote& quote);
    double get_risk_free_rate(const std::string& currency) const;
    void update_risk_free_rate(const std::string& currency, double rate);
    double get_implied_volatility(const std::string& symbol, double expiry) const;
    void update_implied_volatility(const std::string& symbol, const std::vector<VolatilityTerm>& terms);

private:
    std::map<std::string, Quote> quote_;
    std::map<std::string, double> riskfree_rate_;
    std::map<std::string, std::vector<VolatilityTerm>> volatility_surface;
    mutable std::mutex data_mutex;

};