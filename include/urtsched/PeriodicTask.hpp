#pragma once

#include <chrono>

#include <slogger/ILogger.hpp>

#include "BaseTask.hpp"

namespace realtime
{

/** Instances of these are created by the RealtimeKernel::add_periodic() method.
 * They represent tasks that run periodically at a defined interval.
 */
class PeriodicTask : public BaseTask
{
public:
    PeriodicTask(time_utils::ITimer& timer, TaskType tt, const std::string& name,
        const std::chrono::microseconds& t, task_func_t callback,
        logging::ILogger& logger, RealtimeKernel* kernel)
        : BaseTask(timer, tt, name, t, callback, logger, kernel)
    {
    }

    bool overlaps_with(const PeriodicTask& other) const;

    void snapshot_deadline()
    {
        m_snapshot_deadline = time_left_until_deadline();
    }

    std::chrono::nanoseconds get_snapshot_deadline() const
    {
        return m_snapshot_deadline;
    }

private:
    std::chrono::nanoseconds m_snapshot_deadline = std::chrono::nanoseconds(0);
};

} // namespace realtime