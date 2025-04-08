#pragma once

#include "option.hpp"

class EU_Options : public Option {

public:
    EU_Options(double strike, double expiry, const OptionType& type, const ExerciseType& style, std::string underlying);

    double calculate_price(double spot, double rate, double volatility);
    double calculate_gamma(double spot, double rate, double volatility);
    double calculate_theta(double spot, double rate, double volatility);
    double calculate_vega(double spot, double rate, double volatility);
    double calculate_rho(double spot, double rate, double volatility);
};