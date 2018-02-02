#pragma once

#include <cstdlib>
#include <string>
#include <map>
#include <list>
#include <functional>

//#define SERIALIZE_DEBUG

#ifdef SERIALIZE_DEBUG
#include <iostream>
#endif

namespace serialize {
    typedef std::list<std::string> values;

    class map: public std::map<std::string, std::list<std::string>> {
    public:
        typedef std::function<void (const serialize::values& values, const std::string& error)> callback;

        serialize::map& handle(const std::string& keyword, serialize::map::callback handler) {
            auto key_iter = find(keyword);
            bool keyword_exits = key_iter != end();
            if (!keyword_exits)
                handler(serialize::values(), "keyword "+keyword+" not exists");
            else
                handler(key_iter->second, "");
            return *this;
        }

        bool has(const std::string& keyword) {
            return find(keyword) != end();
        }
    };

    serialize::map& args(int argc, char* argv[], serialize::map& smap) {
#ifdef SERIALIZE_DEBUG
        std::cout << "DEBUG: argc = " << argc << "; argv = [" << argv[0];
        for (size_t i = 1; i < argc; ++i)
            std::cout << ", " << argv[i];
        std::cout << ']' << std::endl;
#endif
        smap.clear();
        for (size_t i = 1; i < argc; ++i) {
            std::string argue = argv[i];

            size_t eqpos = argue.find('=');
            if (eqpos == std::string::npos) {
                smap[argue].emplace_front("true");
                continue;
            }
            if (argue.length() == eqpos + 1) continue;

            std::string key = argue.substr(0, eqpos);
            std::string value = argue.substr(eqpos+1);

            smap[key].emplace_front(value);
#ifdef SERIALIZE_DEBUG
            std::cout << "DEBUG: " << "{key: " << key << ", value: " << value << "}" << std::endl;
#endif
        }

        return smap;
    }
}