#pragma once

#include <memory>
#include <vector>

#include <slogger/ILogger.hpp>

#include <urtsched/RealtimeKernel.hpp>
#include <urtsched/ServiceBus.hpp>

namespace realtime
{

class MultiCoreRealtimeKernel
{
public:
    MultiCoreRealtimeKernel(logging::ILogger& logger, service::ServiceBus& bus) : m_bus(bus), m_logger(logger) {}

    std::shared_ptr<RealtimeKernel> add_core()
    {
        auto k = std::make_shared<RealtimeKernel>(m_logger);
        m_bus.add(k);
        m_kernels.push_back(k);
        return k;
    }

    void run(const std::chrono::milliseconds& max_runtime);

    logging::ILogger& get_logger() const { return m_logger; }

private:
    service::ServiceBus& m_bus;
    logging::ILogger& m_logger;

    // one per core:
    std::vector<std::shared_ptr<RealtimeKernel>> m_kernels;

    void reserve_cores();
};

}