/**
 * @file thrhash.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brieif Header definitions for thrhash.c
 */

#ifndef _P3_THRHASH_H
#define _P3_THRHASH_H

#include <thread.h>
#include <x86/page.h>

#define HASH_ENTRIES (PAGE_SIZE / sizeof(thread_t*))

void thrhash_add(thread_t *thr);
void thrhash_remove(thread_t *thr);
thread_t *thrhash_find(unsigned int tid);
unsigned int thrhash_entry(unsigned int tid);

#endif /* _P3_THRHASH_H */
