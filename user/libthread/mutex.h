/**
 * @file mutex.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 * 
 * @brief Header file for mutex.c
 */

#ifndef __P2_MUTEX_H_
#define __P2_MUTEX_H_

#include "p2thrlist.h"

/* Function prototypes */
unsigned int testandset(void *var);

#endif /* !__P2_MUTEX_H_ */