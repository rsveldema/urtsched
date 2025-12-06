#pragma once

#include <chrono>

#include <slogger/ILogger.hpp>

#include "BaseTask.hpp"

namespace realtime
{
class PeriodicTask : public BaseTask
{
public:
    PeriodicTask(TaskType tt, const std::string& name,
        const std::chrono::microseconds& t, task_func_t callback,
        logging::ILogger& logger)
        : BaseTask(tt, name, t, callback, logger)
    {
    }

    bool overlaps_with(const PeriodicTask& other) const;
};

} // namespace realtime