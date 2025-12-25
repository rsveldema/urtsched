#pragma once

#include <optional>
#include <queue>
#include <memory>
#include <map>

#include <slogger/ILogger.hpp>
#include <slogger/ITimer.hpp>

#include <urtsched/RealtimeKernel.hpp>

namespace service
{
class Service : public IService
{
public:
    Service(const std::shared_ptr<realtime::RealtimeKernel>& rt_kernel,
        logging::ILogger& logger)
        : m_rt_kernel(rt_kernel)
        , m_logger(logger)
    {
    }

    time_utils::ITimer& get_timer()
    {
        return m_rt_kernel->get_timer();
    }


    logging::ILogger& get_logger()
    {
        return m_logger;
    }

    std::shared_ptr<realtime::RealtimeKernel>& get_rt_kernel()
    {
        return m_rt_kernel;
    }

    /** if there's nothing to do, call the work pushed with run_oneshot_idle_task()
     */
    void run_oneshot_idle_task(const std::string& name, const realtime::task_func_t& f);

private:
    std::shared_ptr<realtime::RealtimeKernel> m_rt_kernel;
    logging::ILogger& m_logger;

    std::map<std::string, std::shared_ptr<realtime::IdleTask>> m_tasks;
};
} // namespace service