/**
 * @file condvar.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Header file for condvar.c
 */

#ifndef __P2_CONDVAR_H_
#define __P2_CONDVAR_H_

#include "list.h"

/* Function prototypes */
int awaken_first_thread(list_t *list);

#endif /* !__P2_CONDVAR_H_ */