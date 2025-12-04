
#pragma once

#include <memory>
#include <vector>

#include <urtsched/IService.hpp>

namespace service
{

/** all services are registered in this bus
 * and can be queried for their status info
 */
class ServiceBus
{
public:
    std::string get_service_status_as_json();

    void add(const std::shared_ptr<service::IService> s)
    {
        m_services.push_back(s);
    }

private:
    std::vector<std::shared_ptr<service::IService>> m_services;
};
} // namespace service