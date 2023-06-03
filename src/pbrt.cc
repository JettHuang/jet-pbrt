// \brief
//		basic implementation.
//

#include "pbrt.h"

namespace pbrt
{


void log_print(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}


}
