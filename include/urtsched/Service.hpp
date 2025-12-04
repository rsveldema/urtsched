#pragma once

#include <optional>
#include <queue>
#include <memory>

#include <slogger/ILogger.hpp>

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

    logging::ILogger& get_logger()
    {
        return m_logger;
    }

    std::shared_ptr<realtime::RealtimeKernel>& get_rt_kernel()
    {
        return m_rt_kernel;
    }

    /** if there's nothing to do, call the work pushed with push_work_queue()
     */
    void create_idle_handler(const std::string& name);
    void push_work_queue(const realtime::task_func_t& f);

private:
    std::shared_ptr<realtime::RealtimeKernel> m_rt_kernel;
    logging::ILogger& m_logger;

    std::optional<std::shared_ptr<realtime::IdleTask>> m_idle_proxy;

    std::queue<realtime::task_func_t> m_work_queue;
};
} // namespace service