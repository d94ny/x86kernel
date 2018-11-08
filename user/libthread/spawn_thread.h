/**
 * @file spawn_thread.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Header file for spawn_thread.S
 */

#ifndef __P2_SPAWN_THREAD_H_
#define __P2_SPAWN_THREAD_H_

#include <p2thread.h>
#include <syscall.h>
#include <simics.h>
#include <mutex_type.h>

int thr_spawn(void *, void *(*func)(void *), void *, mutex_t *);

#endif /* !__P2_SPAWN_THREAD_H_ */
