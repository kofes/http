//
// sync_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2012 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "client.hpp"
#include "../Salsa20/Salsa20.h"

using boost::asio::ip::tcp;

namespace http {
namespace client {

client::client(int id, const std::string& key): m_id(id) {
    for (size_t i = 0; i < key.length() && i < sizeof(m_key); ++i)
        m_key[i] = (std::uint8_t)key[i];
};

void client::get(
        const std::string &address, const std::string &port,
        const std::string &path, std::stringstream& result) {
    result.clear();
    boost::asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(address, port);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << path << "?id=" << m_id << " HTTP/1.0\r\n";
    request_stream << "Host: " << address << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
        result << "Invalid response\n";
        return;
    }
    if (status_code != 200) {
        result << "Response returned with status code " << status_code << "\n";
        return;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r") {
        result << header << "\n";
    }

    // Write whatever content we already have to output.
    std::stringstream sbuffer;
    if (response.size() > 0)
        sbuffer << &response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error))
        sbuffer << &response;
    // Decode content
    uint8_t buffer[Salsa20::CHUNK_SIZE];
    std::uint8_t nonce[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g' , 'h'};
    while (sbuffer.read(reinterpret_cast<char *>(buffer), Salsa20::CHUNK_SIZE * sizeof(uint8_t)).gcount() > 0) {
        Salsa20::crypt16(m_key, nonce, buffer);
        result.write(reinterpret_cast<char *>(buffer), sbuffer.gcount());
    }

    if (error != boost::asio::error::eof)
        throw boost::system::system_error(error);
    //decode result
}

}
}
