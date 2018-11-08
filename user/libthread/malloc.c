/*
 * these functions should be thread safe.
 * It is up to you to rewrite them
 * to make them thread safe.
 *
 */

#include <stdlib.h>
#include <types.h>
#include <stddef.h>
#include <syscall.h>

#include "p2thread.h"
#include <inc/mutex.h>
#include "mutex.h"

void *malloc(size_t __size)
{
	//while (testandset(&mem_lock)) yield(-1);
	void *result = _malloc(__size);
	//mem_lock = FALSE;
	return result;
}

void *calloc(size_t __nelt, size_t __eltsize)
{
	//while (testandset(&mem_lock)) yield(-1);
	void *result = _calloc(__nelt, __eltsize);
	//mem_lock = FALSE;
	return result;
}

void *realloc(void *__buf, size_t __new_size)
{
	//while (testandset(&mem_lock)) yield(-1);
	void *result = _realloc(__buf, __new_size);
	//mem_lock = FALSE;
	return result;
}

void free(void *__buf)
{
	//while (testandset(&mem_lock)) yield(-1);
	_free(__buf);
	//mem_lock = FALSE;
}
