#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <thread>
#include <unistd.h>

#include <urtsched/MultiCoreRealtimeKernel.hpp>
#include <urtsched/RealtimeKernel.hpp>

#include <slogger/ShellUtils.hpp>
#include <slogger/StringUtils.hpp>


using namespace std::chrono_literals;

namespace realtime
{
/**
 * 1) mkdir /sys/fs/cgroup/cpuset
 * 2) mount -t cgroup -ocpuset cpuset /sys/fs/cgroup/cpuset
 * 3) Create the new cpuset by doing mkdir's and write's (or echo's) in
 * the /sys/fs/cgroup/cpuset virtual file system.
 */

void echo(const std::string& value, const std::string& filename,
    logging::ILogger& logger)
{
    LOG_INFO(logger, "writing {} to {}", value, filename);
    FILE* f = fopen(filename.c_str(), "w");
    if (!f)
    {
        LOG_ERROR(logger, "failed to open {} for writing", filename);
        assert(f);
    }
    fprintf(f, "%s\n", value.c_str());
    fclose(f);
}

void echo(
    const pid_t value, const std::string& filename, logging::ILogger& logger)
{
    echo(std::to_string(value), filename, logger);
}


void MultiCoreRealtimeKernel::reserve_cores_using_cgroups()
{
    std::string CPUSET_PATH = "/sys/fs/cgroup/cpuset/urtsched";
    std::string URTSCHED_PATH = CPUSET_PATH + "/urtsched";

    if (!std::filesystem::exists(URTSCHED_PATH))
    {
        if (!std::filesystem::exists(CPUSET_PATH))
        {
            if (!std::filesystem::create_directories(CPUSET_PATH))
            {
                LOG_ERROR(
                    get_logger(), "failed to create path: {}", CPUSET_PATH);
                abort();
            }
        }

        shell::run_cmd("mount -t cgroup -ocpuset cpuset " + CPUSET_PATH,
            get_logger(), shell::RunOpt::ABORT_ON_ERROR);

        if (!std::filesystem::exists(URTSCHED_PATH))
        {
            if (!std::filesystem::create_directories(URTSCHED_PATH))
            {
                LOG_ERROR(
                    get_logger(), "failed to create path: {}", URTSCHED_PATH);
                abort();
            }
        }
    }
    else
    {
        LOG_INFO(get_logger(), "already created {}", URTSCHED_PATH);
    }

    echo("2-3", URTSCHED_PATH + "/cpuset.cpus", get_logger());
    echo("1", URTSCHED_PATH + "/cpuset.cpu_exclusive", get_logger());
    echo("0", URTSCHED_PATH + "/cpuset.mems", get_logger());
    echo(getpid(), URTSCHED_PATH + "/tasks", get_logger());
}


void MultiCoreRealtimeKernel::run(const std::chrono::milliseconds& max_runtime)
{
    switch (m_reserve_cores)
    {
    case CoreReservationMechanism::CGROUPS:
        reserve_cores_using_cgroups();
        break;
    case CoreReservationMechanism::TASKSET:
        // todo: check that we're actually on the cores we expect to be
        LOG_INFO(get_logger(), "pls use taskset cmd to pre-place the service");
        break;
    case CoreReservationMechanism::NONE:
        LOG_INFO(get_logger(),
            "assuming service cpu reservation is managed via some other way");
        break;
    }

    assert(!m_kernels.empty());

    std::vector<std::thread> threads;
    for (unsigned i = 1; i < m_kernels.size(); i++)
    {
        threads.emplace_back(
            [this, max_runtime, i]() { m_kernels[i]->run(max_runtime); });
    }

    m_kernels[0]->run(max_runtime);

    for (auto& t : threads)
    {
        t.join();
    }
}


} // namespace realtime