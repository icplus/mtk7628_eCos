#ifndef __HELPERD_H__
#define __HELPERD_H__
//#define NDEBUG
#include <stdio.h>
#include <assert.h>

#ifndef NDEBUG
#define DEBUG(_fmt, _args...) \
	printf("[ DEBUG ] [ %s:%u ] "_fmt, __func__, __LINE__, ##_args)
#define INFO(_fmt, _args...) \
	printf("[ INFO ][ %s:%u ] "_fmt, __func__, __LINE__, ##_args)
#define WARN(_fmt, _args...) \
	printf("[ WARN ][ %s:%u ] "_fmt, __func__, __LINE__, ##_args)
#define ERROR(_fmt, _args...) \
	printf("[ ERROR ][ %s:%u ] "_fmt, __func__, __LINE__, ##_args)
#else
#define DEBUG(_fmt, _args...) 
#define INFO(_fmt, _args...) \
	printf("[ INFO ] "_fmt, ##_args)
#define WARN(_fmt, _args...) \
	printf("[ WARN ] "_fmt, ##_args)
#define ERROR(_fmt, _args...) \
	printf("[ ERROR ] "_fmt, ##_args)
#endif /* NDEBUG */

#endif /* __HELPERD_H__ */
