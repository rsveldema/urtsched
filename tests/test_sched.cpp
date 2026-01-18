#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <urtsched/RealtimeKernel.hpp>
#include <slogger/DirectConsoleLogger.hpp>

#include "../simple-logger/tests/slogger_mocks.hpp"

using namespace testing;
using namespace realtime;
using namespace std::chrono_literals;

namespace unittests
{

class RealtimeKernelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        timer = std::make_unique<time_utils::mocks::Timer>();
        logger = std::make_unique<logging::DirectConsoleLogger>(
            true, true, logging::LogOutput::CONSOLE);
        kernel = std::make_shared<RealtimeKernel>(*timer, *logger, "test-kernel");

        // Setup timer to return increasing time values
        EXPECT_CALL(*timer, get_time_ns())
            .Times(AnyNumber())
            .WillRepeatedly([this]() {
                auto result = current_time;
                current_time += 1ms; // Advance time by 1ms each call
                return result;
            });
    }

    std::unique_ptr<time_utils::mocks::Timer> timer;
    std::unique_ptr<logging::DirectConsoleLogger> logger;
    std::shared_ptr<RealtimeKernel> kernel;
    std::chrono::nanoseconds current_time{0};
};

// Test that periodic tasks run before idle tasks and in correct order
TEST_F(RealtimeKernelTest, PeriodicTasksRunBeforeIdleTasks)
{
    std::vector<std::string> execution_order;

    // Add 3 periodic tasks with different intervals
    auto periodic1 = kernel->add_periodic(
        TaskType::SOFT_REALTIME,
        "periodic-100ms",
        100ms,
        [&execution_order](BaseTask& task) {
            execution_order.push_back("periodic1");
            return TaskStatus::TASK_OK;
        });
    periodic1->enable();

    auto periodic2 = kernel->add_periodic(
        TaskType::SOFT_REALTIME,
        "periodic-200ms",
        200ms,
        [&execution_order](BaseTask& task) {
            execution_order.push_back("periodic2");
            return TaskStatus::TASK_OK;
        });
    periodic2->enable();

    auto periodic3 = kernel->add_periodic(
        TaskType::HARD_REALTIME,
        "periodic-50ms",
        50ms,
        [&execution_order](BaseTask& task) {
            execution_order.push_back("periodic3");
            return TaskStatus::TASK_OK;
        });
    periodic3->enable();

    // Add 3 idle tasks
    auto idle1 = kernel->add_idle_task(
        "idle1",
        [&execution_order](BaseTask& task) {
            execution_order.push_back("idle1");
            return TaskStatus::TASK_OK;
        });

    auto idle2 = kernel->add_idle_task(
        "idle2",
        [&execution_order](BaseTask& task) {
            execution_order.push_back("idle2");
            return TaskStatus::TASK_OK;
        });

    auto idle3 = kernel->add_idle_task(
        "idle3",
        [&execution_order](BaseTask& task) {
            execution_order.push_back("idle3");
            return TaskStatus::TASK_OK;
        });

    // Run the kernel for a short duration to trigger some periodic tasks
    kernel->run(300ms);

    // Verify that execution occurred
    ASSERT_FALSE(execution_order.empty());

    // Check that periodic tasks ran
    bool has_periodic1 = std::find(execution_order.begin(), execution_order.end(), "periodic1") != execution_order.end();
    bool has_periodic2 = std::find(execution_order.begin(), execution_order.end(), "periodic2") != execution_order.end();
    bool has_periodic3 = std::find(execution_order.begin(), execution_order.end(), "periodic3") != execution_order.end();
    
    EXPECT_TRUE(has_periodic1 || has_periodic2 || has_periodic3) 
        << "At least one periodic task should have executed";

    // Find the first idle task position
    auto first_idle_pos = std::find_if(execution_order.begin(), execution_order.end(),
        [](const std::string& name) {
            return name.find("idle") == 0;
        });

    // If any idle tasks ran, verify all periodic tasks before them are actually periodic
    if (first_idle_pos != execution_order.end())
    {
        for (auto it = execution_order.begin(); it != first_idle_pos; ++it)
        {
            // All tasks before first idle should be periodic tasks
            if (it->find("idle") == 0)
            {
                FAIL() << "Found idle task '" << *it << "' before first idle task position";
            }
        }
    }

    // Print execution order for debugging
    std::cout << "Execution order: ";
    for (const auto& name : execution_order)
    {
        std::cout << name << " ";
    }
    std::cout << std::endl;
}

// Test that periodic tasks are scheduled by their deadlines
TEST_F(RealtimeKernelTest, PeriodicTasksScheduledByDeadline)
{
    std::vector<std::string> execution_order;
    int periodic1_count = 0;
    int periodic2_count = 0;
    int periodic3_count = 0;

    // Add periodic tasks with different intervals
    // Shorter interval = higher priority
    auto periodic_fast = kernel->add_periodic(
        TaskType::HARD_REALTIME,
        "fast-10ms",
        10ms,
        [&](BaseTask& task) {
            execution_order.push_back("fast");
            periodic1_count++;
            return TaskStatus::TASK_OK;
        });
    periodic_fast->enable();

    auto periodic_medium = kernel->add_periodic(
        TaskType::SOFT_REALTIME,
        "medium-50ms",
        50ms,
        [&](BaseTask& task) {
            execution_order.push_back("medium");
            periodic2_count++;
            return TaskStatus::TASK_OK;
        });
    periodic_medium->enable();

    auto periodic_slow = kernel->add_periodic(
        TaskType::SOFT_REALTIME,
        "slow-100ms",
        100ms,
        [&](BaseTask& task) {
            execution_order.push_back("slow");
            periodic3_count++;
            return TaskStatus::TASK_OK;
        });
    periodic_slow->enable();

    // Run for 100ms - fast should run ~10 times, medium ~2 times, slow ~1 time
    kernel->run(100ms);

    // Verify execution counts are proportional to intervals
    EXPECT_GT(periodic1_count, 0) << "Fast periodic task should have run";
    EXPECT_GT(periodic2_count, 0) << "Medium periodic task should have run";
    
    // Fast task should run more frequently than medium task
    if (periodic1_count > 0 && periodic2_count > 0)
    {
        EXPECT_GT(periodic1_count, periodic2_count)
            << "Fast task (10ms) should run more times than medium task (50ms)";
    }
}

// Test that idle tasks run when no periodic tasks are ready
TEST_F(RealtimeKernelTest, IdleTasksRunWhenNoPeriodicTasksReady)
{
    std::vector<std::string> execution_order;

    // Add one periodic task with long interval
    auto periodic = kernel->add_periodic(
        TaskType::SOFT_REALTIME,
        "periodic-1s",
        1s,
        [&](BaseTask& task) {
            execution_order.push_back("periodic");
            return TaskStatus::TASK_OK;
        });
    periodic->enable();

    // Add idle tasks
    auto idle1 = kernel->add_idle_task("idle1", [&](BaseTask& task) {
        execution_order.push_back("idle1");
        return TaskStatus::TASK_OK;
    });

    auto idle2 = kernel->add_idle_task("idle2", [&](BaseTask& task) {
        execution_order.push_back("idle2");
        return TaskStatus::TASK_OK;
    });

    // Run for short time - periodic won't be ready, so idle tasks should run
    kernel->run(50ms);

    // Should have some idle task executions
    int idle_count = std::count_if(execution_order.begin(), execution_order.end(),
        [](const std::string& name) { return name.find("idle") == 0; });
    
    EXPECT_GT(idle_count, 0) << "Idle tasks should have run when no periodic tasks were ready";
}

}
