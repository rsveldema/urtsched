#pragma once

#include <string>

#include <iuring/Error.hpp>

namespace service
{
    class IService
    {
    public:
        virtual std::string get_service_status_as_json() const { return ""; }

        [[nodiscard]]
        virtual iuring::Error init() = 0;

        [[nodiscard]]
        virtual iuring::Error finish() = 0;
    };
}