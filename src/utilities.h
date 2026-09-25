#ifndef UTILITIES_H
#define UTILITIES_H

#include <stddef.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	_x < _y ? _x : _y; })
#define max(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	_x > _y ? _x : _y; })
#define _min(x,y) min(x,y)
#define _max(x,y) max(x,y)

/* dosemu2 runs these from its plugin loader; we call them from main() */
#define CONSTRUCTOR2(n) void n##_plugin_init(void)
int tempname(char *tmpl, size_t x_suffix_len);

#endif
