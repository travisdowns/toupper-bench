/**
 * Implementation of various very basic algorithms, mostly as a litmus test for the more complicated ones.
 */

#include <stdlib.h>
#include <ctype.h>

void toupper_rawloop(char* buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buf[i] = toupper(buf[i]);
    }
}
