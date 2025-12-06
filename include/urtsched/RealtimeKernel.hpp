#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>

#include <slogger/ILogger.hpp>
#include <slogger/TimeUtils.hpp>

#include <urtsched/IService.hpp>

#include "BaseTask.hpp"
#include "IdleTask.hpp"
#include "PeriodicTask.hpp"

namespace realtime
{
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
        TaskType tt,
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

    std::vector<std::shared_ptr<PeriodicTask>> get_next_periodics();

    // return the task thats earliest:
    std::shared_ptr<PeriodicTask> get_earliest_next_periodic();

    /**return a list of tasks whose runtime can overlap 'next'.
    * the returned list contains 'next' as well.
    */
    std::vector<std::shared_ptr<PeriodicTask>> get_periodics_that_can_overlap(const std::shared_ptr<PeriodicTask>& next);

    /** return a sorted list of real-time tasks */
    std::vector<std::shared_ptr<PeriodicTask>> get_sorted_realtime_tasks(const std::vector<std::shared_ptr<PeriodicTask>>& next_up);

    void run_idle_tasks();
};

} // namespace realtime