#pragma once

#include <functional>
#include <string>
#include <vector>

#include <slogger/ILogger.hpp>
#include <slogger/TimeUtils.hpp>

#include "task_defs.hpp"

namespace realtime
{
class BaseTask
{
public:
    BaseTask(TaskType tt, const std::string& name,
        const std::chrono::microseconds& t, task_func_t callback,
        logging::ILogger& logger)
        : m_task_type(tt)
        , m_interval(t)
        , m_task_func(callback)
        , m_timeout(std::chrono::microseconds(0))
        , m_name(name)
        , m_logger(logger)
    {
    }

    TaskType get_task_type() const
    {
        return m_task_type;
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

    /** called to wait for deadline to elapse because there's no more idle tasks
     * that we can squeeze into the time until this task needs to run.
     * We only need to do this for hard-realtime tasks as soft ones can run when
     * we have time for them afterwards.
     */
    void wait_for_deadline() const
    {
        assert(get_task_type() == TaskType::HARD_REALTIME);
        while (!m_timeout.elapsed())
        {
            ;
        }
    }

    void run_elapsed()
    {
        if (get_task_type() == TaskType::HARD_REALTIME)
        {
            assert(m_timeout.elapsed());
        }

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

    /** if you need the most accuracy */
    std::chrono::nanoseconds max_time_taken_ns() const
    {
        return m_max_time_taken;
    }

    /** if you need the most accuracy */
    std::chrono::nanoseconds warmup_max_time_taken_ns() const
    {
        return m_warmup_max_time_taken;
    }

    std::chrono::microseconds max_time_taken_us() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>( m_max_time_taken );
    }

    std::chrono::microseconds warmup_max_time_taken_us() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(  m_warmup_max_time_taken );
    }

    std::chrono::nanoseconds average_time_taken_ns() const
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
           average_time_taken_us());
    }

    std::chrono::microseconds average_time_taken_us() const
    {
        return std::chrono::microseconds(
            (int64_t) ((double) m_total_time_taken_us.count() / m_num_calls));
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
    TaskType m_task_type;
    std::chrono::microseconds m_interval = std::chrono::microseconds(0);
    std::chrono::nanoseconds m_max_time_taken = std::chrono::nanoseconds(0);
    // to avoid overflow, lets use micros here:
    std::chrono::microseconds m_total_time_taken_us = std::chrono::microseconds(0);
    std::chrono::nanoseconds m_warmup_max_time_taken =
        std::chrono::nanoseconds(0);

    uint64_t m_num_calls = 0;
    task_func_t m_task_func;
    time_utils::Timeout m_timeout;
    bool m_enabled = false;
    std::string m_name;
    logging::ILogger& m_logger;
};

} // namespace realtime