#pragma once

#include <chrono>
#include <functional>

/**
 * @brief Helper: lambda with defined real-time for evaluation
 */
class Event
{
public:
  using EventCallback = std::function<void()>;
  using TimePoint = std::chrono::high_resolution_clock::time_point;

  Event() = default;

  Event(std::chrono::milliseconds ms, EventCallback callback);

  Event(EventCallback callback);

  const TimePoint& getDeadline() const;
  EventCallback getCallback() const;

  bool operator<(const Event& other) const
  {
    return this->deadline > other.deadline;
  }

private:
  TimePoint deadline;
  EventCallback callback;
};
