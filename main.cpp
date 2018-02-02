#include <iostream>
#include <map>
#include <list>
#include <sstream>
#include <string>
#include <fstream>

#include "args_serializer.h"
#include "server/server.hpp"
#include "client/client.hpp"

using namespace std;

int main(int argc, char* argv[]) {
    serialize::map smap;
    map<int, string> clients;
    list<string> route;
    string address;
    string root_dir;
    uint16_t port = 8080;

    serialize::args(argc, argv, smap)
    .handle("address", [&] (const serialize::values& values, const std::string& error) {
        address = !values.empty() ? values.front() : "127.0.0.1";
    })
    .handle("port", [&] (const serialize::values& values, const std::string& error) {
        for (const std::string& value : values) {
            std::stringstream buffer(value);
            buffer >> port;
            if (port) break;
        }
        if (!port) port = 8080;
    })
    .handle("clients", [&] (const serialize::values& values, const std::string& error) {
        std::ifstream fin;
        for (const std::string& filename: values) {
            fin.open(filename);
            if (!fin.is_open()) continue;
            {
                size_t id;
                string key;
                while (fin >> id >> key) clients.emplace(id, key);
            }
                fin.close();
        }

    })
    .handle("root", [&] (const serialize::values& values, const std::string& error) {
        root_dir = !values.empty() ? values.front() : "web/";
    })
    .handle("path", [&] (const serialize::values& values, const std::string& error) {
        for (const std::string& path: values)
            path != "true" ? route.emplace_back(path) : void();
    });

    if (smap.has("client") && smap.has("server")) {
        cout << "error: two conflicting options [client, server]" << endl;
        return 1;
    }

    if (smap.has("server"))
        try {
            http::server::server server(address, std::to_string(port), root_dir);
            server.run();
        } catch (exception& e) {
            cout << "exception: " << e.what() << endl;
        }
    else try {
            http::client::client client;
            for (const std::string& path: route) {
                stringstream response;
                client.get(address, std::to_string(port), path, response);
                cout << "<--! response from '"+ path +"' -->" << endl;
                cout << response.str() << endl;
            }
        } catch (exception& e) {
            cout << "exception: " << e.what() << endl;
        }
    return 0;
}