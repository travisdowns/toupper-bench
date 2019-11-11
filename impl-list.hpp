#ifndef IMPL_LIST_H_
#define IMPL_LIST_H_

#include "common-cxx.hpp"

#include <stddef.h>
#include <string>
#include <vector>


using to_upper_fn = void (char* buffer, size_t size);

to_upper_fn toupper_rawloop;
to_upper_fn toupper_rawloop_algo;
to_upper_fn toupper_transform;
to_upper_fn toupper_branch;
to_upper_fn toupper_lookup;

enum AlgoFlags {
    NONE         = 0,
    /** algo is too slow to run at default array sizes */
    SLOW         = 1 << 0,
    /** algo doesn't return the right result (e.g., because it is a dummy for testing) */
    INCORRECT    = 1 << 1
};

struct test_description {
    const char *name;
    to_upper_fn *f;
    const char *desc;
    AlgoFlags flags;

    void call_f(const bench_args& args) const {
        f(args.buffer, args.size);
    }
};

/**
 * Return the benchmark exactly matching the given name, or nullptr
 * if not found.
 */
const test_description* get_by_name(const std::string& name);

/**
 * Given a comma separated list of test names, return a list of all the
 * tests, or throw if one isn't found.
 */
std::vector<test_description> get_by_list(const std::string& list);

/**
 * Return all test descriptors.
 */
const std::vector<test_description>& get_all();

#endif
