#pragma once

namespace realtime
{
using task_func_t = std::function<void()>;

enum class TaskType
{
    HARD_REALTIME,
    SOFT_REALTIME
};

} // namespace realtime