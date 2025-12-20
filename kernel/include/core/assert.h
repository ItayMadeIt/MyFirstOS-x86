#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <stdbool.h>

void assert(bool must_be_true);

static inline void __assert_header_marker(void) {}

#endif // __ASSERT_H__