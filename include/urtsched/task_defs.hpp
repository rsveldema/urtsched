#pragma once

namespace realtime
{
enum class TaskStatus
{
    TASK_OK,
    TASK_YIELD
};

class BaseTask;

using task_func_t = std::function<TaskStatus(BaseTask&)>;

enum class TaskType
{
    HARD_REALTIME,
    SOFT_REALTIME
};

} // namespace realtime