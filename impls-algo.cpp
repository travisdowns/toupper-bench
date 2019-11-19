/**
 * Implementation of various very basic algorithms, mostly as a litmus test for the more complicated ones.
 */

#include <algorithm>
#include <ctype.h>
#include <stdlib.h>

void toupper_rawloop_algo(char* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = toupper(buf[i]);
    }
}

void toupper_transform(char* buf, size_t size) {
    std::transform(buf, buf + size, buf, toupper);
}

void toupper_branch(char* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        char c = buf[i];
        if (c >= 'a' && c <= 'z') {
            buf[i] = c - 32;
        }
    }
}

struct ctype_luts {
    char toupper_lut[256];
};

ctype_luts make_lut() {
    ctype_luts ret;
    for (unsigned i = 0; i < 256; i++) {
        ret.toupper_lut[i] = toupper(i);
    }
    return ret;
}

ctype_luts global_lut = make_lut();

char toupper_ascii(char c) {
    return global_lut.toupper_lut[(unsigned)c];
}

void toupper_lookup(char* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = toupper_ascii(buf[i]);
    }
}
