#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <source_location>
#include <concepts>
#include <format>

#include "core/Utility.hpp"

namespace azm
{

enum class LogVerbosity : uint8_t
{
    NoLogging = 0,
    Display,
    Warning,
    Error,
    Log,
    Fatal
};

struct LogCategory
{
    explicit LogCategory(const std::string& name) : m_name(name) {}
    std::string name() const { return m_name; }

private:
    const std::string m_name;
};

class Log final : public NonCopyable
{
public:
    static Log& getInstance()
    {
        static Log instance;
        return instance;
    }

    // !! Do not use the direct call; it's unsafe. Use macro version with static checks !!
    void log(const LogCategory& category,  //
        LogVerbosity verbosity,            //
        const std::string& message,        //
        bool showLocation = false,         //
        const std::source_location location = std::source_location::current()) const;

private:
    Log();
    ~Log();

    class Impl;
    std::unique_ptr<Impl> m_pImpl;
};

constexpr LogVerbosity c_minVerbosity = LogVerbosity::Display;
constexpr LogVerbosity c_maxVerbosity = LogVerbosity::Fatal;

// concepts
template <typename T>
concept ValidLogCategory = std::constructible_from<LogCategory, T>;

template <typename T>
concept LoggableMessage = std::convertible_to<T, std::string> || std::convertible_to<T, std::string_view>;

template <LogVerbosity V>
concept ValidVerbosityLevel = V == LogVerbosity::NoLogging   //
                              || V == LogVerbosity::Display  //
                              || V == LogVerbosity::Warning  //
                              || V == LogVerbosity::Error    //
                              || V == LogVerbosity::Log      //
                              || V == LogVerbosity::Fatal;

}  // namespace azm

#define DEFINE_LOG_CATEGORY_STATIC(logName)       \
    namespace                                     \
    {                                             \
    const azm::LogCategory logName(#logName); \
    }

#define AZM_LOG_IMPL(categoryName, verbosity, showLocation, formatStr, ...)                                                        \
    do                                                                                                                            \
    {                                                                                                                             \
        if constexpr (azm::LogVerbosity::verbosity >= azm::c_minVerbosity &&                                              \
                      azm::LogVerbosity::verbosity <= azm::c_maxVerbosity)                                                \
        {                                                                                                                         \
            static_assert(azm::ValidVerbosityLevel<azm::LogVerbosity::verbosity>,                                         \
                "Verbosity must be one of: NoLogging, Display, Warning, Error, Log, Fatal");                                      \
            static_assert(azm::ValidLogCategory<decltype(categoryName)>, "Category must be of type LogCategory");             \
            static_assert(                                                                                                        \
                azm::LoggableMessage<decltype(formatStr)>, "Message must be convertible to std::string or std::string_view"); \
            azm::Log::getInstance().log(                                                                                      \
                categoryName, azm::LogVerbosity::verbosity, std::format(formatStr __VA_OPT__(, ) __VA_ARGS__), showLocation); \
        }                                                                                                                         \
    } while (0)

#define AZM_LOG(categoryName, verbosity, formatStr, ...) AZM_LOG_IMPL(categoryName, verbosity, false, formatStr, __VA_ARGS__)
#define AZM_LOG_DEBUG(categoryName, verbosity, formatStr, ...) AZM_LOG_IMPL(categoryName, verbosity, true, formatStr, __VA_ARGS__)