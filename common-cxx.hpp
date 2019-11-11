#ifndef SI_COMMON_CXX_H_
#define SI_COMMON_CXX_H_

/**
 * The C++ specific common stuff.
 */

/* to align with C++20 std::span */
#define span_CONFIG_INDEX_TYPE size_t

#include "hedley.h"
#include "inttypes.h"
#include "nonstd/span.hpp"

#include <stdlib.h>

/**
 * Bundles all the arguments.
 */
struct bench_args {
    char* buffer;
    size_t size;
};

using char_span = nonstd::span<char>;

#endif
