#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h> //pthread_t
#include <sched.h>   //cpu_set_t , CPU_SET

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <thread>

#include <urtsched/RealtimeKernel.hpp>

#include <slogger/ILogger.hpp>
#include <slogger/StringUtils.hpp>
#include <slogger/TimeUtils.hpp>

using namespace std::chrono_literals;


namespace realtime
{
void RealtimeKernel::set_sched_affinity(uint32_t core)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);      // clears the cpuset
    CPU_SET(core, &cpuset); // set CPU 2 on cpuset

    /* bind process to processor 0 */
    if (int status = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset); status < 0)
    {
        LOG_ERROR(get_logger(), "failed to set thread affinity: %s", strerror(-status));
    }
}


[[nodiscard]] std::shared_ptr<PeriodicTask> RealtimeKernel::add_periodic(
    TaskType tt, const std::string& name,
    const std::chrono::microseconds& interval, const task_func_t& callback)
{
    auto s = std::make_shared<PeriodicTask>(m_timer,
        tt, "periodic: " + name, interval, callback, m_logger, this);
    for (size_t i = 0; i < m_periodic_list.size(); i++)
    {
        if (m_periodic_list[i] == nullptr)
        {
            m_periodic_list[i] = s;
            s->disable();
            return s;
        }
    }
    m_periodic_list.push_back(s);
    s->disable();
    return s;
}


bool RealtimeKernel::remove(const std::shared_ptr<PeriodicTask>& task_ptr)
{
    for (size_t ix = 0; ix < m_periodic_list.size(); ix++)
    {
        if (m_periodic_list[ix] == task_ptr)
        {
            m_periodic_list[ix] = nullptr;
            return true;
        }
    }
    return false;
}

std::shared_ptr<IdleTask> RealtimeKernel::add_idle_task(
    const std::string& name, const task_func_t& callback)
{
    auto s = std::make_shared<IdleTask>(m_timer,
        "idle: " + name, 0us, callback, m_logger, this);

    for (size_t i = 0; i < m_idle_list.size(); i++)
    {
        if (m_idle_list[i] == nullptr)
        {
            m_idle_list[i] = s;
            s->enable();
            return s;
        }
    }

    m_idle_list.push_back(s);
    s->enable();
    return s;
}


void RealtimeKernel::run_idle_tasks()
{
    for (auto& t : m_idle_list)
    {
        if (t)
        {
            if (t->is_enabled())
            {
                t->run();
            }
        }
    }
}


std::shared_ptr<PeriodicTask> RealtimeKernel::get_earliest_next_periodic()
{
    std::shared_ptr<PeriodicTask> next;

    for (auto& t : m_periodic_list)
    {
        if (!t)
        {
            continue;
        }
        if (!t->is_enabled())
        {
            // LOG_INFO(get_logger() "discarding: {} = disabled\n",
            //     t->get_name());
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

bool PeriodicTask::overlaps_with(const PeriodicTask& other) const
{
    const auto t = other.time_left_until_deadline();
    const auto my_start = time_left_until_deadline();
    const auto my_end = my_start + max_time_taken_ns();
    return t >= my_start and t <= my_end;
}


std::vector<std::shared_ptr<PeriodicTask>>
RealtimeKernel::get_periodics_that_can_overlap(
    const std::shared_ptr<PeriodicTask>& next)
{
    std::vector<std::shared_ptr<PeriodicTask>> ret;

    for (auto& t : m_periodic_list)
    {
        if (!t)
        {
            continue;
        }
        if (!t->is_enabled())
        {
            // LOG_INFO(get_logger() "discarding: {} = disabled\n",
            //     t->get_name());
            continue;
        }

        if (t == next)
        {
            ret.push_back(t);
            continue;
        }

        if (t->overlaps_with(*next) || next->overlaps_with(*t))
        {
            ret.push_back(t);
        }
    }


    return ret;
}


std::vector<std::shared_ptr<PeriodicTask>> RealtimeKernel::get_next_periodics()
{
    auto next = get_earliest_next_periodic();
    if (next == nullptr)
    {
        return {};
    }

    return get_periodics_that_can_overlap(next);
}


std::vector<std::shared_ptr<PeriodicTask>>
RealtimeKernel::get_sorted_realtime_tasks(
    const std::vector<std::shared_ptr<PeriodicTask>>& next_up)
{
    std::vector<std::shared_ptr<PeriodicTask>> ret;
    for (const auto& it : next_up)
    {
        if (it->get_task_type() == TaskType::HARD_REALTIME)
        {
            ret.push_back(it);
        }
    }

    std::sort(ret.begin(), ret.end(),
        [](const std::shared_ptr<PeriodicTask>& t1,
            const std::shared_ptr<PeriodicTask>& t2) {
            return t1->time_left_until_deadline() <
                t2->time_left_until_deadline();
        });
    return ret;
}


void RealtimeKernel::step()
{
    auto next_up = get_next_periodics();
    if (next_up.empty())
    {
        run_idle_tasks();
        return;
    }

    bool ran_some_idle_tasks = false;

    while (next_up[0]->have_time_left_before_deadline())
    {
        for (auto& t : m_idle_list)
        {
            if (!t)
            {
                continue;
            }
            if (t->is_enabled())
            {
                if (next_up[0]->time_left_until_deadline() >
                    t->max_time_taken_ns())
                {
                    ran_some_idle_tasks = true;
                    t->run();
                }
            }
        }
    }


    if (!ran_some_idle_tasks)
    {
        if (m_idle_list.empty())
        {
            // no idle tasks to run, so its ok.
        }
        else
        {
            static int missed_idle_runs;
            if (missed_idle_runs++ > 100)
            {
                missed_idle_runs = 0;
                LOG_ERROR(get_logger(),
                    "something amis ({}): failed to run idle tasks for too "
                    "long",
                    m_name);
            }
        }
    }

    // run the hard-realtime tasks first to give them priority:
    {
        const auto realtime_tasks = get_sorted_realtime_tasks(next_up);
        if (m_debug)
        {
            int ix = 0;
            for (auto& it : realtime_tasks)
            {
                LOG_INFO(get_logger(),
                    "rt-sched[{}] called {} at {} (max: {}, avg {})", ix,
                    it->get_name(), it->time_left_until_deadline(),
                    it->max_time_taken_us(), it->average_time_taken_us());
                ix++;
            }
        }
        for (auto& it : realtime_tasks)
        {
            it->wait_for_deadline();
            it->run_elapsed();
        }
    }

    // lets be fair and run the soft-realtime tasks
    for (auto& it : next_up)
    {
        if (it->get_task_type() != TaskType::HARD_REALTIME)
        {
            it->run_elapsed();
        }
    }
}

void RealtimeKernel::run(const std::chrono::milliseconds& runtime)
{
    time_utils::Timeout t{m_timer, runtime};
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
    const auto warmup_micros_max = warmup_max_time_taken_us();
    const double warmup_max_flt =
        (warmup_micros_max.count() / (1000.0 * 1000.0));

    const auto micros_max = max_time_taken_us();
    const double max_flt = (micros_max.count() / (1000.0 * 1000.0));

    const auto micros_avg = average_time_taken_us();
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