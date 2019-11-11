#include "impl-list.hpp"
#include "common-cxx.hpp"
#include "misc.hpp"


const test_description all_funcs[] = {
    {"toupper_rawloop", toupper_rawloop, "Raw C-style loop", NONE},
    {"toupper_rawloop_algo", toupper_rawloop_algo, "Raw C-style loop after <algo>", NONE},
    {"toupper_transform", toupper_transform, "C toupper after including <algorithm>", NONE},
    {"toupper_branch", toupper_branch, "Branchy ASCII", NONE},
    {"toupper_lookup", toupper_lookup, "Lookup ASCII", NONE},
};

auto b() -> decltype(get_all().begin()) {
    return get_all().begin();
}

auto e() -> decltype(get_all().end()) {
    return get_all().end();
}

const test_description* get_by_name(const std::string& name) {
    auto it = std::find_if(b(), e(), [&](auto d) { return name == d.name; });
    return it == e() ? nullptr : &*it;
}

std::vector<test_description> get_by_list(const std::string& list) {
    std::vector<test_description> ret;
    for (auto& name : split(list, ",")) {
        auto t = get_by_name(name);
        if (!t) {
            throw std::runtime_error("no test named " + name);
        }
        ret.push_back(*t);
    }
    return ret;
}

const std::vector<test_description>& get_all() {
    static std::vector<test_description> all =
            std::vector<test_description>(all_funcs, all_funcs + COUNT_OF(all_funcs));
    return all;
}
