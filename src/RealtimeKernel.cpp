#include <urtsched/RealtimeKernel.hpp>

#include <slogger/ILogger.hpp>
#include <slogger/TimeUtils.hpp>
#include <slogger/StringUtils.hpp>

#include <algorithm>
#include <cstdio>
#include <thread>

using namespace std::chrono_literals;

static constexpr auto WARMUP_COUNT = 5;

namespace realtime
{
void BaseTask::run()
{
    m_num_calls++;
    const auto start = time_utils::get_current_time();
    m_task_func();
    const auto end = time_utils::get_current_time();
    assert(end >= start); // overflow?
    auto took = end - start;

    if (took > 1ms)
    {
        const auto micros =
            std::chrono::duration_cast<std::chrono::microseconds>(took);

        LOG_ERROR(get_logger(), "task[{}] took too long: %g millis",
            m_name.c_str(), (micros.count() / 1000.0));
        took = took /
            20; // lie a bit to make sure this task can be scheduled at all.
    }

    m_total_time_taken +=
        std::chrono::duration_cast<std::chrono::microseconds>(took);

    if (m_num_calls < WARMUP_COUNT)
    {
        if (took > m_warmup_max_time_taken)
        {
            m_warmup_max_time_taken = took;
        }
    }
    else
    {
        if (took > m_max_time_taken)
        {
            m_max_time_taken = took;
        }
    }
}


[[nodiscard]] std::shared_ptr<PeriodicTask> RealtimeKernel::add_periodic(
    const std::string& name, const std::chrono::microseconds& interval,
    const task_func_t& callback)
{
    auto s = std::make_shared<PeriodicTask>(
        "periodic-" + name, interval, callback, m_logger);
    m_periodic_list.emplace_back(s);
    s->disable();
    return s;
}


bool RealtimeKernel::remove_periodic(const std::string& name)
{
    for (size_t i = 0; i < m_periodic_list.size(); ++i)
    {
        if (m_periodic_list[i]->get_name() == "periodic-" + name)
        {
            m_periodic_list.erase(m_periodic_list.begin() + i);
            return true;
        }
    }
    return false;
}


std::shared_ptr<IdleTask> RealtimeKernel::add_idle_task(
    const std::string& name, const task_func_t& callback)
{
    auto s =
        std::make_shared<IdleTask>("idle-" + name, 0us, callback, m_logger);
    m_idle_list.emplace_back(s);
    s->enable();
    return s;
}


void RealtimeKernel::run_idle_tasks()
{
    for (auto& t : m_idle_list)
    {
        if (t->is_enabled())
        {
            t->run();
        }
    }
}


std::shared_ptr<PeriodicTask> RealtimeKernel::get_next_periodic()
{
    std::shared_ptr<PeriodicTask> next;
    assert(next == nullptr);

    for (auto& t : m_periodic_list)
    {
        if (!t->is_enabled())
        {
            //fprintf(stderr, "discarding: {} = disabled\n",
            //    t->get_name().c_str());
            continue;
        }

        if (next == nullptr)
        {
            next = t;
            continue;
        }

        if (t->time_left_until_deadline() < next->time_left_until_deadline())
        {
            next = t;
        }
    }

    return next;
}


void RealtimeKernel::step()
{
    auto next_up = get_next_periodic();
    if (next_up == nullptr)
    {
        run_idle_tasks();
        return;
    }
    assert(next_up != nullptr);

    while (next_up->have_time_left_before_deadline())
    {
        for (auto& t : m_idle_list)
        {
            if (t->is_enabled())
            {
                if (next_up->time_left_until_deadline() > t->max_time_taken())
                {
                    t->run();
                }
            }
            else
            {
                fprintf(
                    stderr, "idle task is disabled: {}\n", t->get_name().c_str());
            }
        }
    }

    next_up->run_elapsed();
}

void RealtimeKernel::run(const std::chrono::milliseconds& runtime)
{
    time_utils::Timeout t{ runtime };
    while (!should_exit())
    {
        step();

        if (runtime.count() > 0)
        {
            if (t.elapsed())
            {
                break;
            }
        }
    }
}


std::string BaseTask::get_service_status_as_json() const
{
    const auto warmup_max = warmup_max_time_taken();
    const auto warmup_micros_max =
        std::chrono::duration_cast<std::chrono::microseconds>(warmup_max);
    const double warmup_max_flt =
        (warmup_micros_max.count() / (1000.0 * 1000.0));

    const auto max = max_time_taken();
    const auto micros_max =
        std::chrono::duration_cast<std::chrono::microseconds>(max);
    const double max_flt = (micros_max.count() / (1000.0 * 1000.0));

    const auto avg = average_time_taken();
    const auto micros_avg =
        std::chrono::duration_cast<std::chrono::microseconds>(avg);
    const double avg_flt = (micros_avg.count() / (1000.0 * 1000.0));


    return std::format(
        "{{ \"name\": \"{}\", \"max\": {},  \"warmup\": {},  \"avg\": {} }}",
        get_name(), max_flt, warmup_max_flt, avg_flt);
}


std::string RealtimeKernel::get_service_status_as_json() const
{
    std::string ret = "\"tasks\": [";
    const char* comma = "";
    for (const auto& p : m_periodic_list)
    {
        ret += comma;
        ret += p->get_service_status_as_json() + "\n";
        comma = ",";
    }
    for (const auto& p : m_idle_list)
    {
        ret += comma;
        ret += p->get_service_status_as_json() + "\n";
        comma = ",";
    }
    ret += "]";
    return ret;
}


} // namespace realtime