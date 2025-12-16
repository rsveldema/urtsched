#pragma once

#include <memory>
#include <vector>

#include <slogger/ILogger.hpp>

#include <urtsched/RealtimeKernel.hpp>
#include <urtsched/ServiceBus.hpp>

namespace realtime
{
enum class CoreReservationMechanism
{
    NONE,
    TASKSET,
    CGROUPS
};


class MultiCoreRealtimeKernel
{
public:
    /** when reserve-cores is true, it will use cgroups to reserve cores
     * for the service. You can turn this off if you've instead have some other way to schedule the service.
     * For example, under linux you can start the kernel with isolcpus=2,3 and then at runtime
     * start the service with 'taskset --cpu-list 2,3'.
     */
    MultiCoreRealtimeKernel(
        logging::ILogger& logger, service::ServiceBus& bus, CoreReservationMechanism reserve_cores, uint32_t first_reserved_core)
        : m_bus(bus)
        , m_logger(logger)
        , m_reserve_cores(reserve_cores)
        , m_first_reserved_core(first_reserved_core)
    {
    }

    std::shared_ptr<RealtimeKernel> add_core()
    {
        auto k = std::make_shared<RealtimeKernel>(
            m_logger, "core-" + std::to_string(m_kernels.size()));
        m_bus.add(k);
        m_kernels.push_back(k);
        return k;
    }

    void run(const std::chrono::milliseconds& max_runtime);

    logging::ILogger& get_logger() const
    {
        return m_logger;
    }

private:
    service::ServiceBus& m_bus;
    logging::ILogger& m_logger;
    const CoreReservationMechanism m_reserve_cores;
    const uint32_t m_first_reserved_core;

    // one per core:
    std::vector<std::shared_ptr<RealtimeKernel>> m_kernels;

    void reserve_cores_using_cgroups();
};

} // namespace realtime