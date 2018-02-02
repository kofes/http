#pragma once

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>
#include <boost/asio.hpp>

namespace http {
namespace client {
class client {
public:
    client(const client&) = delete;
    client& operator=(const client&) = delete;
    client() = default;
    void get(
            const std::string& address, const std::string& port,
            const std::string& path, std::stringstream& response);
};
}
}