#include <cassert>

#include <urtsched/Service.hpp>


namespace service
{

void Service::run_oneshot_idle_task(
    const std::string& name, const realtime::task_func_t& f)
{
    auto it = m_tasks.find(name);
    if (it != m_tasks.end())
    {
        it->second->enable();
        return;
    }

    auto task = get_rt_kernel()->add_idle_task(
        "async-task-" + name, [this, f](realtime::BaseTask& t) {
            t.disable();
            return f(t);
        });

    m_tasks[name] = task;

    task->enable();
}

} // namespace service