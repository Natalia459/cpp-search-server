#pragma once
#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)

class LogDuration {
public:
    using Clock = std::chrono::steady_clock;
    LogDuration(const std::string& operation) : operation_(operation) 
    {
    }
    LogDuration(const std::string& operation, std::ostream& out) : operation_(operation), out_(out)
    {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_ << operation_ << ": "s << duration_cast<nanoseconds>(dur).count() << " ns"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string operation_;
    std::ostream& out_ = std::cerr;
};