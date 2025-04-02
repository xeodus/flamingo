#include "order_book.hpp";
#include <chrono>
#include <ctime>

void OrderBook::prune_good_for_day() {
    using namespace std::chrono;
    const auto end = hours(16);

    while (true) {
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);
        std::tm now_parts;
        localtime_r(&now_c, &now_parts);

        if (now_parts.tm_hour >= end.count()) {
            now_parts.tm_mday += 1;
        }

        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        auto next = system_clock::from_time_t(mktime(&now_parts));
        auto till = next - now + milliseconds(100);

    }
}