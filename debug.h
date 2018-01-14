#ifndef __DEBUG_H_
#define __DEBUG_H_

#include <stdio.h>

namespace n_debug {

extern bool is_output;
extern FILE *output_file;

void output( const char *format, ... );

#ifndef NDEBUG
#define DEBUG_MSG(format, ...) (::n_debug::output( __FILE__ ":%d " format, __LINE__, __VA_ARGS__ ))
#else
#define DEBUG_MSG(format, ...)
#endif

}

#endif
