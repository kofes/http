//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"
#include "../Salsa20/Salsa20.h"

namespace http {
namespace server {

request_handler::request_handler(const std::string& doc_root, const std::map<int, std::string>& clients)
  : doc_root_(doc_root), m_clients(clients) {}

void request_handler::handle_request(const request& req, reply& rep) {
    // Decode url to path & params
    std::string request_path;
    std::map<std::string, std::string> params;
    if (!url_decode(req.uri, request_path, params)) {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

    // Read client's id & key
    std::uint8_t client_key[16] = {0};
    {
        int client_id;
        if (params.find("id") == params.end()) {
            rep = reply::stock_reply(reply::bad_request);
            return;
        }
        std::stringstream tmp_stream(params["id"]);
        tmp_stream >> client_id;
        if (m_clients.find(client_id) == m_clients.end()) {
            rep = reply::stock_reply(reply::bad_request);
            return;
        }
        const std::string& ref_key = m_clients[client_id];
        for (size_t i = 0; i < ref_key.length(); ++i)
            client_key[i] = (std::uint8_t)ref_key[i];
    }

    // Request path must be absolute and not contain "..".
    if (request_path.empty() || request_path[0] != '/' ||
        request_path.find("..") != std::string::npos) {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

    // If path ends in slash (i.e. is a directory) then add "index.html".
    if (request_path[request_path.size() - 1] == '/') {
        request_path += "index.html";
    }

    // Determine the file extension.
    std::size_t last_slash_pos = request_path.find_last_of("/");
    std::size_t last_dot_pos = request_path.find_last_of(".");
    std::string extension;
    if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos) {
        extension = request_path.substr(last_dot_pos + 1);
    }

    // Open the file to send back.
    std::string full_path = doc_root_ + request_path;
    std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
    if (!is) {
        rep = reply::stock_reply(reply::not_found);
        return;
    }

    // Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    char buf[512];
    while (is.read(buf, sizeof(buf)).gcount() > 0) {
        // Encrypt content
        for (size_t i = 0; i < 512 / Salsa20::CHUNK_SIZE; ++i) {
            std::uint8_t nonce[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g' , 'h'};
            Salsa20::crypt16(
                    client_key,
                    nonce,
                    reinterpret_cast<std::uint8_t *>(buf+i*Salsa20::CHUNK_SIZE)
            );
        }
        rep.content.append(buf, is.gcount());
    }

    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    rep.headers[0].value = std::to_string(rep.content.size());
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = mime_types::extension_to_type(extension);

    /// WARNING!!! client can't resolve content-type without content-type field!
    // Encrypt header's values
//    for (header& header: rep.headers) {
//        for (size_t i = 0; i < header.value.length() / Salsa20::CHUNK_SIZE; ++i) {
//            char buffer[Salsa20::CHUNK_SIZE] = {0};
//            for (size_t k = 0; k < Salsa20::CHUNK_SIZE; ++k)
//                buffer[k] = header.value[i * Salsa20::CHUNK_SIZE + k];
//            std::uint8_t nonce[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
//            Salsa20::crypt16(
//                    client_key,
//                    nonce,
//                    reinterpret_cast<std::uint8_t *>(buffer)
//            );
//            for (size_t k = 0; k < Salsa20::CHUNK_SIZE; ++k)
//                header.value[i * Salsa20::CHUNK_SIZE + k] = buffer[k];
//        }
//        if (header.value.length() % Salsa20::CHUNK_SIZE) {
//            char buffer[Salsa20::CHUNK_SIZE] = {0};
//            for (size_t k = 0; k < header.value.length() % Salsa20::CHUNK_SIZE; ++k)
//                buffer[k] = header.value[(header.value.length() / Salsa20::CHUNK_SIZE) *
//                                         Salsa20::CHUNK_SIZE + k];
//            std::uint8_t nonce[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
//            Salsa20::crypt16(
//                    client_key,
//                    nonce,
//                    reinterpret_cast<std::uint8_t *>(buffer)
//            );
//            for (size_t k = 0; k < header.value.length() % Salsa20::CHUNK_SIZE; ++k)
//                header.value[(header.value.length() / Salsa20::CHUNK_SIZE) *
//                                     Salsa20::CHUNK_SIZE + k] = buffer[k];
//        }
//    }
}

bool request_handler::url_decode(
        const std::string& in,
        std::string& out,
        std::map<std::string, std::string>& smap
) {
    out.clear();
    out.reserve(in.size());
    std::size_t ind = 0;

    bool (*read_str_before)(
            const std::string& src,
            const std::size_t spos,
            std::string& value,
            const std::string& ends,
            const std::string& errs
    ) = [] (
            const std::string& src,
            const std::size_t spos,
            std::string& value,
            const std::string& ends,
            const std::string& errs
    ) -> bool {
        value.clear();
        std::size_t suppos = src.length();
        for (char end: ends) {
            std::size_t tmppos = src.find(end, spos+1);
            if (tmppos != std::string::npos && suppos > tmppos)
                suppos = tmppos;
        }

        for (size_t i = spos; i < suppos; ++i)
            value.push_back(src[i]);

        for (char err: errs)
            if (err == '\0' && suppos == src.length() ||
                value.find(err) != std::string::npos)
                return true;
        return false;
    };

    bool (*decode_hex)(
            const std::string& src,
            std::string& res
    ) = [](
            const std::string& src,
            std::string& res
    ) -> bool {
        std::string tmp;
        for (std::size_t i = 0; i < src.size(); ++i) {
            if (src[i] == '%') {
                if (i + 3 <= src.size()) {
                    int value = 0;
                    std::istringstream is(src.substr(i + 1, 2));

                    if (is >> std::hex >> value) {
                        tmp += static_cast<char>(value);
                        i += 2;
                    } else
                        return true;
                } else
                    return true;
            } else if (src[i] == '+') tmp += ' ';
            else tmp += src[i];
        }

        res = tmp;
        return false;
    };

    std::string tmp;
    bool err_flag;
    err_flag = read_str_before(in, ind, tmp, {'?', '\0', '#'}, {});
    ind += tmp.length();
    err_flag |= decode_hex(tmp, tmp);
    if (err_flag) return false;
    out += tmp;

    if (in[ind] == '?') {
        ++ind;
        do {
            std::string key;
            std::string value;
            bool err_flag;

            //read key
            err_flag = read_str_before(in, ind, key, {'='}, {'&', '#', '\0', '?', '/'});
            err_flag |= decode_hex(key, key);
            if (err_flag) return false;
            ind += key.length()+1;
            if (ind >= in.size()) return false;

            //read value
            err_flag = read_str_before(in, ind, value, {'\0', '&', '#'}, {'?', '/'});
            err_flag |= decode_hex(value, value);
            if (err_flag) return false;
            ind += value.length() + 1;

            //insert (key, value) pair
            auto it = smap.find(key);
            if (it == smap.end())
                smap[key] = value;
        } while (ind < in.size() && in[ind] != '#');
    }

    if (in[ind] == '#') {
        out += in[ind++];
        std::string tmp;
        bool err_flag = decode_hex(in, tmp);
        if (err_flag) return false;
        out += tmp;
    }

    return true;
}

} // namespace server
} // namespace http