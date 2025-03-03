#include "event.hpp"

Event::Event(std::chrono::milliseconds ms, EventCallback callback)
  : deadline(std::chrono::high_resolution_clock::now() + ms)
  , callback(std::move(callback))
{
}

Event::Event(EventCallback callback)
  : Event(std::chrono::milliseconds(0), callback)
{
}

const Event::TimePoint&
Event::getDeadline() const
{
  return deadline;
}

Event::EventCallback
Event::getCallback() const
{
  return callback;
}
