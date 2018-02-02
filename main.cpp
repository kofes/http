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
    string address;
    uint16_t port = 8080;
    //only for server
    string root_dir;
    map<int, string> clients;
    //only for client
    list<string> route;
    int client_id;
    std::string client_key;

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
        if (!fin.is_open()) {
            fin.open("clients.txt");
            if (fin.is_open()) {
                size_t id;
                string key;
                while (fin >> id >> key) clients.emplace(id, key);
            }
            fin.close();
        }
    })
    .handle("root", [&] (const serialize::values& values, const std::string& error) {
        root_dir = !values.empty() ? values.front() : "web";
    })
    .handle("path", [&] (const serialize::values& values, const std::string& error) {
        for (const std::string& path: values)
            path != "true" ? route.emplace_back(path) : void();
    })
    .handle("id", [&] (const serialize::values& values, const std::string& error) {
        client_id = -1;
        for (const std::string& value : values) {
            std::stringstream buffer(value);
            buffer >> client_id;
            if (client_id >= 0) break;
        }
    })
    .handle("key", [&] (const serialize::values& values, const std::string& error) {
        client_key = !values.empty() ? values.front() : "";
    });

    if (smap.has("client") && smap.has("server")) {
        cout << "error: two conflicting options [client, server]" << endl;
        return 1;
    }

    if (smap.has("server"))
        try {
            http::server::server server(address, std::to_string(port), root_dir, clients);
            server.run();
        } catch (exception& e) {
            cout << "exception: " << e.what() << endl;
        }
    else {
        if (client_id < 0) {
            cout << "error: client id not set" << endl;
            return 1;
        }
        if (client_key.empty()) {
            cout << "error: client key not set" << endl;
            return 2;
        }
        try {
            http::client::client client(client_id, client_key);
            for (const std::string& path: route) {
                stringstream response;
                client.get(address, std::to_string(port), path, response);
                cout << "<--! response from '"+ path +"' -->" << endl;
                cout << response.str() << endl;
            }
        } catch (exception& e) {
            cout << "exception: " << e.what() << endl;
        }
    }
    return 0;
}