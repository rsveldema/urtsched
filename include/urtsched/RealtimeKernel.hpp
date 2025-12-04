#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

#include <slogger/ILogger.hpp>
#include <slogger/TimeUtils.hpp>

#include <urtsched/IService.hpp>

namespace realtime
{
class RealtimeKernel;

using task_func_t = std::function<void()>;

class BaseTask
{
public:
    BaseTask(const std::string& name, const std::chrono::microseconds& t,
        task_func_t callback, logging::ILogger& logger)
        : m_interval(t)
        , m_task_func(callback)
        , m_timeout(std::chrono::microseconds(0))
        , m_name(name)
        , m_logger(logger)
    {
    }

    logging::ILogger& get_logger()
    {
        return m_logger;
    }

    std::string get_service_status_as_json() const;

    const std::string& get_name() const
    {
        return m_name;
    }

    void run();

    void run_elapsed()
    {
        assert(m_timeout.elapsed());

        m_timeout = time_utils::Timeout(m_interval);

        run();
    }

    bool have_time_left_before_deadline() const
    {
        return !m_timeout.elapsed();
    }

    std::chrono::nanoseconds time_left_until_deadline() const
    {
        assert(this != nullptr);
        return m_timeout.time_left();
    }

    std::chrono::nanoseconds max_time_taken() const
    {
        return m_max_time_taken;
    }

    std::chrono::nanoseconds warmup_max_time_taken() const
    {
        return m_warmup_max_time_taken;
    }

    std::chrono::microseconds average_time_taken() const
    {
        return std::chrono::microseconds(
            (int64_t) ((double) m_total_time_taken.count() / m_num_calls));
    }

    void set_period(const std::chrono::microseconds& t)
    {
        m_interval = t;
    }

    void enable()
    {
        m_enabled = true;
    }

    void disable()
    {
        m_enabled = false;
    }

    bool is_enabled() const
    {
        return m_enabled;
    }

private:
    std::chrono::microseconds m_interval = std::chrono::microseconds(0);
    std::chrono::nanoseconds m_max_time_taken = std::chrono::nanoseconds(0);
    std::chrono::microseconds m_total_time_taken = std::chrono::microseconds(0);
    std::chrono::nanoseconds m_warmup_max_time_taken =
        std::chrono::nanoseconds(0);

    uint64_t m_num_calls = 0;
    task_func_t m_task_func;
    time_utils::Timeout m_timeout;
    bool m_enabled = false;
    std::string m_name;
    logging::ILogger& m_logger;
};

class IdleTask : public BaseTask
{
public:
    IdleTask(const std::string& name, const std::chrono::microseconds& t,
        task_func_t callback, logging::ILogger& logger)
        : BaseTask(name, t, callback, logger)
    {
    }
};

class PeriodicTask : public BaseTask
{
public:
    PeriodicTask(const std::string& name, const std::chrono::microseconds& t,
        task_func_t callback, logging::ILogger& logger)
        : BaseTask(name, t, callback, logger)
    {
    }
};


/** schedules stuff on a single core.
 * As its for a single core only, it does not need
 * locks/synchronization code and is therefore really fast.
 * We do not support between code task migration as a consequence.
 */
class RealtimeKernel : public service::IService
{
public:
    RealtimeKernel(logging::ILogger& logger)
        : m_logger(logger)
    {
    }

    [[nodiscard]] error::Error init() override
    {
        return error::Error::OK;
    }

    [[nodiscard]] error::Error finish() override
    {
        return error::Error::OK;
    }

    [[nodiscard]] std::shared_ptr<PeriodicTask> add_periodic(
        const std::string& name, const std::chrono::microseconds& interval,
        const task_func_t& callback);

    [[nodiscard]] std::shared_ptr<IdleTask> add_idle_task(
        const std::string& name, const task_func_t& callback);


    /** return true on successful removal */
    bool remove_periodic(const std::string& name);


    bool should_exit() const
    {
        return false;
    }

    /** @param max_runtime if 0 return forever
     */
    void run(const std::chrono::milliseconds& max_runtime);

    void step();

    std::string get_service_status_as_json() const;

    logging::ILogger& get_logger()
    {
        return m_logger;
    }

private:
    logging::ILogger& m_logger;
    std::vector<std::shared_ptr<PeriodicTask>> m_periodic_list;
    std::vector<std::shared_ptr<IdleTask>> m_idle_list;

    std::shared_ptr<PeriodicTask> get_next_periodic();

    void run_idle_tasks();
};

} // namespace realtime