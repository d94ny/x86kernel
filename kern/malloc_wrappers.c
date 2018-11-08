/**
 * @file malloc_wrappers.c
 * @author Loic Ottet (lottet)
 * @author Daniel Balle (dballe)
 * 
 * @brief Contains thread safe versions of malloc functions
 */

#include <stddef.h>
#include <malloc.h>
#include <malloc_internal.h>
#include <simics.h>
#include <lock.h>
#include <thread.h>

/* safe versions of malloc functions */
void *malloc(size_t size)
{
	mutex_lock(&mem_lock);	
	void *result = _malloc(size);
	mutex_unlock(&mem_lock);
	return result;
}

void *memalign(size_t alignment, size_t size)
{
	mutex_lock(&mem_lock);
	void *result = _memalign(alignment, size);
	mutex_unlock(&mem_lock);
	return result;
}

void *calloc(size_t nelt, size_t eltsize)
{
	mutex_lock(&mem_lock);
	void *result = _calloc(nelt, eltsize);
	mutex_unlock(&mem_lock);
	return result;
}

void *realloc(void *buf, size_t new_size)
{
	mutex_lock(&mem_lock);
	void *result = _realloc(buf, new_size);
	mutex_unlock(&mem_lock);
	return result;
}

void free(void *buf)
{
	mutex_lock(&mem_lock);
	_free(buf);
	mutex_unlock(&mem_lock);
}

void *smalloc(size_t size)
{
	mutex_lock(&mem_lock);
	void *result = _smalloc(size);
	mutex_unlock(&mem_lock);
	return result;
}

void *smemalign(size_t alignment, size_t size)
{
	mutex_lock(&mem_lock);
	void *result = _smemalign(alignment, size);
	mutex_unlock(&mem_lock);
	return result;
}

void sfree(void *buf, size_t size)
{
	mutex_lock(&mem_lock);
	_sfree(buf, size);
	mutex_unlock(&mem_lock);
}


