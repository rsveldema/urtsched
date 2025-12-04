#pragma once

#include <string>

#include <slogger/Error.hpp>

namespace service
{
    class IService
    {
    public:
        virtual std::string get_service_status_as_json() const { return ""; }

        [[nodiscard]]
        virtual error::Error init() = 0;

        [[nodiscard]]
        virtual error::Error finish() = 0;
    };
}