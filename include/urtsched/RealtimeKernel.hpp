#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

#include <slogger/ILogger.hpp>
#include <slogger/TimeUtils.hpp>
#include <slogger/ITimer.hpp>

#include <urtsched/IService.hpp>
#include <urtsched/fixed_size_vector.hpp>

#include "BaseTask.hpp"
#include "IdleTask.hpp"
#include "PeriodicTask.hpp"

namespace realtime
{
/** Schedules stuff on a single core.
 * As its for a single core only, it does not need
 * locks/synchronization code and is therefore really fast.
 * We do not support between code task migration as a consequence.
 */
class RealtimeKernel : public service::IService
{
public:
    RealtimeKernel(time_utils::ITimer& timer, logging::ILogger& logger, const std::string& name)
        : m_timer(timer), m_logger(logger), m_name(name)
    {
    }

    time_utils::ITimer& get_timer()
    {
        return m_timer;
    }

    void set_sched_affinity(uint32_t core);

    [[nodiscard]] error::Error init() override
    {
        return error::Error::OK;
    }

    [[nodiscard]] error::Error finish() override
    {
        return error::Error::OK;
    }

    /** returns the name of the kernel */
    const std::string& get_name() const { return m_name;}


    /** Add a periodic task to the scheduler.
     * The returned task is disabled by default.
     * Therefore, call periodic->enable() to enable it.
     */
    [[nodiscard]] std::shared_ptr<PeriodicTask> add_periodic(
        TaskType tt,
        const std::string& name, const std::chrono::microseconds& interval,
        const task_func_t& callback);

    /** Add an idle task to the scheduler.
     * It is enabled by default.
     */
    [[nodiscard]] std::shared_ptr<IdleTask> add_idle_task(
        const std::string& name, const task_func_t& callback);

    /** return true on successful removal */
    bool remove(const std::shared_ptr<PeriodicTask>& task_ptr);

    bool should_exit() const
    {
        return false;
    }

    /** @param max_runtime if no value is provided or if 0 run forever
     */
    void run(std::optional<const std::chrono::milliseconds> max_runtime);

    void step();

    std::string get_service_status_as_json() const;

    logging::ILogger& get_logger()
    {
        return m_logger;
    }

private:
    time_utils::ITimer& m_timer;
    static constexpr bool m_debug = false;
    logging::ILogger& m_logger;
    const std::string m_name;

    static constexpr auto MAX_PERIODIC_TASKS = 64;
    static constexpr auto MAX_IDLE_TASKS = 16;

    realtime::fixed_size_vector<std::shared_ptr<PeriodicTask>, MAX_PERIODIC_TASKS> m_periodic_list;
    realtime::fixed_size_vector<std::shared_ptr<IdleTask>, MAX_IDLE_TASKS> m_idle_list;

    std::vector<std::shared_ptr<PeriodicTask>> get_next_periodics();

    // return the task thats earliest:
    std::shared_ptr<PeriodicTask> get_earliest_next_periodic();

    /**return a list of tasks whose runtime can overlap 'next'.
    * the returned list contains 'next' as well.
    */
    std::vector<std::shared_ptr<PeriodicTask>> get_periodics_that_can_overlap(const std::shared_ptr<PeriodicTask>& next);

    /** return a sorted list of real-time tasks */
    std::vector<PeriodicTask*> get_sorted_realtime_tasks(const std::vector<std::shared_ptr<PeriodicTask>>& next_up);

    void run_idle_tasks();
};

} // namespace realtime