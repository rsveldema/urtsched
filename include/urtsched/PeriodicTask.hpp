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
    PeriodicTask(TaskType tt, const std::string& name,
        const std::chrono::microseconds& t, task_func_t callback,
        logging::ILogger& logger, RealtimeKernel* kernel)
        : BaseTask(tt, name, t, callback, logger, kernel)
    {
    }

    bool overlaps_with(const PeriodicTask& other) const;
};

} // namespace realtime