/**
 * @file launch.h
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 *
 * @brief Function prototype for the laucher
 */

#ifndef __P3_LAUNCH_H_
#define __P3_LAUNCH_H_

/**
 * @brief Switches from kernel mode to user mode
 *
 * launch() is used to simulate the transition back into user mode when
 * there was no user-to-kernel transition to begin with. Such a situation
 * occurs everytime we want to run a thread we just created.
 *
 * To transition to user mode we create the stack an INT instruction would 
 * have formed and call IRET.
 *
 * The stack will be a bit unusual since it was handcrafted. When the
 * second part of the context_switch returns to launch the stack has the
 * following structure
 *
 * 	+ ------------- +
 * 	|      esp3		| 
 * 	+ ------------- +
 * 	|     entry		|
 * 	+ ------------- +
 * 	|      self     | <-- esp
 * 	+ ------------- +
 *  | ret to launch |
 *  + ------------- +
 *
 * self was used by the context_switch as an argument to set_running.
 * Then after the RET from the context_switch our esp points to self.
 * So we can push anything (like the ebp) to get the impression of a "sane"
 * stack. Now esp and entry are accessed through 12(ebp) and 8(ebp)
 *
 * @param entry the entry point of the program
 * @param esp3 the user stack to which we switch
 */
void launch(unsigned long entry, uint32_t esp3);

#endif /* !__P3_LAUCH_H */
