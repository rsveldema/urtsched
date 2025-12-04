#include <cassert>

#include <urtsched/Service.hpp>


namespace service
{
void Service::create_idle_handler(const std::string& name)
{
    m_idle_proxy = get_rt_kernel()->add_idle_task("async-handler-" + name, [this]() {

        if (m_work_queue.empty())
        {
            return;  
        }

        auto func = m_work_queue.back();
        m_work_queue.pop();
        func();
    });
}

void Service::push_work_queue(const realtime::task_func_t& f)
{
    // should have a task to process the
    // work queue items
    assert(m_idle_proxy.has_value());
    m_work_queue.push(f);
}

} // namespace service