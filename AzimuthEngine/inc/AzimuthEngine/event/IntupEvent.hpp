#pragma once

namespace azm
{

enum class EventType
{
    WindowClose,
    WindowResize,
    MouseMove,
    MouseButton,
    MouseScroll,
    KeyPress
};

struct EventData
{

};

struct InputEvent
{
    EventType type;
    EventData data;
};

}