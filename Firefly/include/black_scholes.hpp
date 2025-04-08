#pragma once 

#include <tuple>

class BlackScholes {
    static std::tuple<double, double> calculate_D1D2(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_price(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_price(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_delta(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_gamma(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_theta(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_theta(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_vega(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_call_rho(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_delta(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_gamma(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_theta(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_vega(double spot, double strike, double rate, double volatility, double time_to_maturity);
    static double calculate_put_rho(double spot, double strike, double rate, double volatility, double time_to_maturity);
};