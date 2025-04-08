#pragma once

#include <string>

enum class OptionType {
    Call,
    Put
};

enum class ExerciseType {
    American,
    EU
};

class Option {

public:
    Option(double strike, double expiry, const OptionType& type, const ExerciseType& style, std::string underlying)
    : strike_{strike}, expiry_{expiry}, type_{type}, style_{style}, underlying_{underlying} {}

    double get_strike() const { return strike_; }
    double get_expiry() const { return expiry_; }
    const OptionType& get_type() const { return type_; }
    const ExerciseType& get_style() const { return style_; }
    std::string get_underlying() { return underlying_; }

    double calculate_price(double spot, double rate, double volatility);
    double calculate_gamma(double spot, double rate, double volatility);
    double calculate_theta(double spot, double rate, double volatility);
    double calculate_vega(double spot, double rate, double volatility);
    double calculate_rho(double spot, double rate, double volatility);

private:
    double strike_;
    double expiry_;
    OptionType type_;
    ExerciseType style_;
    std::string underlying_;
    
};