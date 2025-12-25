#include <urtsched/RealtimeKernel.hpp>

#include <slogger/ILogger.hpp>
#include <slogger/StringUtils.hpp>
#include <slogger/TimeUtils.hpp>

#include <algorithm>
#include <cstdio>
#include <thread>

using namespace std::chrono_literals;

static constexpr auto WARMUP_COUNT = 5;

static constexpr auto MAX_ALLOWED_TASK_TIME = 500us;


namespace realtime
{
void BaseTask::run()
{
    m_num_calls++;
    const auto start = m_timer.get_time_ns();
    const auto task_status = m_task_func(*this);
    const auto end = m_timer.get_time_ns();
    assert(end >= start); // overflow?
    auto took = end - start;

    if (took > MAX_ALLOWED_TASK_TIME)
    {
        const auto micros =
            std::chrono::duration_cast<std::chrono::microseconds>(took);

        const auto avg = average_time_taken_ns();
        LOG_ERROR(get_logger(),
            "{} - task[{}] took too long: {}, avg = {}, calls = {}, ok = {}",
            m_kernel->get_name(), m_name, micros, avg, m_num_calls,
            m_num_task_ok_calls);

        // lie a bit to make sure this task can be scheduled at all:
        took = took / 20;
    }

    m_total_time_taken_us +=
        std::chrono::duration_cast<std::chrono::microseconds>(took);

    if (task_status == TaskStatus::TASK_YIELD)
    {
        // we yielded, so do not count this time towards our stats.
        return;
    }

    m_num_task_ok_calls++;

    if (m_num_calls < WARMUP_COUNT)
    {
        if (took > m_warmup_max_time_taken)
        {
            m_warmup_max_time_taken = took;
        }
    }
    else
    {
        if (took > MAX_ALLOWED_TASK_TIME)
        {
            // lets not count towards our normal statistics.
            return;
        }

        if (took > m_max_time_taken)
        {
            m_max_time_taken = took;
        }
    }
}

} // namespace realtime