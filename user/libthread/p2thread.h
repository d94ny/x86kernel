/**
 * @file p2thread.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Header file for p2thread.h
 */

#ifndef __P2_P2THREAD_H_
#define __P2_P2THREAD_H_

#include <p2thrlist.h>
#include <mutex_type.h>
#include <inc/types.h>

#define MAX_THREAD_STACK 128
#define MIN_THREAD_ID 32
#define MAX_THREAD_ID 9999

void thr_launch(void *(*func)(void *), void *, mutex_t *);
thrdesc_t *thr_newdesc(int, unsigned int);
thrdesc_t *thr_getdesc(void);

extern boolean_t mem_lock;

#endif /* !__P2_P2THREAD_H_ */
