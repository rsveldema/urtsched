#include <string>

#include <urtsched/ServiceBus.hpp>

namespace service
{
    std::string ServiceBus::get_service_status_as_json()
    {
        std::string ret;
        const char* comma = "";
        for (auto& s : m_services)
        {
            if (const auto& k = s->get_service_status_as_json(); ! k.empty())
            {
                ret += comma;

                if (ret.empty())
                {
                    ret = k;
                } else {
                    ret += k;
                }
                comma = ",";
            }
        }
        return ret;
    }

} // namespace service