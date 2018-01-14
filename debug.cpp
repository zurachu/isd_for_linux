#include "debug.h"

#include <stdarg.h>

namespace n_debug {

bool is_output = true;
FILE *output_file = stderr;

void output( const char *format, ... )
{
	if ( is_output == false )
		return;

	va_list ap;
	va_start( ap, format );

	vfprintf( output_file, format, ap );

	va_end( ap );
}

}
