/*
 * @file syscall.h
 * @brief Headers for syscall functions
 * 
 * @author Daniel Balle (dballe)
 * @author Loic Ottet (lottet)
 */

#ifndef __KERN_SYSCALL_H_
#define __KERN_SYSCALL_H_

int install_syscalls();

int gettid_int(void);
int _gettid(void);

int exec_int(void);
int _exec(void **args);

int fork_int(void);
int _fork(void);

int yield_int(void);
int _yield(int tid);

int deschedule_int(void);
int _deschedule(int *flag);

int make_runnable_int(void);
int _make_runnable(int tid);

unsigned int get_ticks_int(void);
unsigned int _get_ticks(void);

int sleep_int(void);
int _sleep(int ticks);

void set_status_int(void);
void _set_status(int status);

int wait_int(void);
int _wait(int *status_ptr);

void vanish_int(void);
void _vanish(void);

int new_pages_int(void);
int _new_pages(int *args);

int remove_pages_int(void);
int _remove_pages(void *base);

int getchar_int(void);
int _getchar(void);

int readline_int(void);
int _readline(void** args);

int print_int(void);
int _print(void** args);

int set_term_color_int(void);
int _set_term_color(int color);

int get_cursor_pos_int(void);
int _get_cursor_pos(int** args);

int set_cursor_pos_int(void);
int _set_cursor_pos(int* args);

void halt_int(void);
void _halt(void);

int swexn_int(void);
int _swexn(void **args);

int thread_fork_int(void);
int _thread_fork(void);

int _readfile(void **args);
int readfile_int(void **args);

#endif /* __KERN_SYSCALL_H_ */
