#pragma once

#include <string>
#include "log/log.hpp"

DEFINE_LOG_CATEGORY_STATIC(LogEvent);

namespace azm
{

template<typename... Args>
class Event
{
public: 
    void invoke(Args... args)
    {
        AZM_LOG(LogEvent, Display, "Dispatch event");
    }
};
    
}