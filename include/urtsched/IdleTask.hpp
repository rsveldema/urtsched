#pragma once

#include <string>
#include <chrono>

#include <slogger/ILogger.hpp>

#include "BaseTask.hpp"

namespace realtime
{
class IdleTask : public BaseTask
{
public:
    IdleTask(const std::string& name, const std::chrono::microseconds& t,
        task_func_t callback, logging::ILogger& logger)
        : BaseTask(TaskType::SOFT_REALTIME, name, t, callback, logger)
    {
    }
};
} // namespace realtime