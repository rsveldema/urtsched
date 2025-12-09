#pragma once

#include <chrono>
#include <string>

#include <slogger/ILogger.hpp>

#include "BaseTask.hpp"

namespace realtime
{

/** Instances of these are created by the RealtimeKernel::add_idle() method.
 * They represent tasks that run when no periodic tasks are scheduled to run.
 */
class IdleTask : public BaseTask
{
public:
    IdleTask(const std::string& name, const std::chrono::microseconds& t,
        task_func_t callback, logging::ILogger& logger, RealtimeKernel* kernel)
        : BaseTask(TaskType::SOFT_REALTIME, name, t, callback, logger, kernel)
    {
    }
};
} // namespace realtime