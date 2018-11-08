###########################################################################
# This is the include file for the make file.
# You should have to edit only this file to get things to build.
###########################################################################

###########################################################################
# Tab stops
###########################################################################
# If you use tabstops set to something other than the international
# standard of eight characters, this is your opportunity to inform
# our print scripts.
TABSTOP = 4

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = afs

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# blanked.  Your kernel MUST BOOT AND RUN if 410TESTS and STUDENTTESTS
# are blank.  It would be wise for you to test that this works.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory.
#
410TESTS = peon merchant actual_wait exec_basic exec_basic_helper exec_nonexist fork_bomb fork_exit_bomb fork_test1 getpid_test1 loader_test1 loader_test2 sleep_test1 yield_desc_mkrun deschedule_hang cho cho2 wait_getpid wild_test1 print_basic knife cat cho_variant chow ck1 coolness fork_wait fork_wait_bomb halt_test make_crash make_crash_helper mem_eat_test mem_permissions new_pages slaughter stack_test1 work remove_pages_test1 remove_pages_test2 readline_basic swexn_basic_test swexn_cookie_monster swexn_dispatch swexn_regs swexn_stands_for_swextensible swexn_uninstall_test ack fib bistromath

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory.
#
STUDENTTESTS = 

###########################################################################
# Data files provided by course staff to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the 410user/files
# directory.
#
410FILES =

###########################################################################
# Data files you have created which you wish to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the user/files
# directory.
#
STUDENTFILES =

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o panic.o condvar.o mutex.o p2thread.o p2thrlist.o spawn_thread.o mutex_asm.o list.o semaphore.o rwlock.o exception.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = syscall.o gettid.o exec.o fork.o yield.o sleep.o make_runnable.o deschedule.o get_ticks.o vanish.o wait.o set_status.o new_pages.o remove_pages.o getchar.o readline.o print.o set_term_color.o get_cursor_pos.o set_cursor_pos.o halt.o swexn.o thread_fork.o readfile.o

###########################################################################
# Object files for your automatic stack handling
###########################################################################
AUTOSTACK_OBJS = autostack.o

###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o
#
# Kernel object files you provide in from kern/
#
KERNEL_OBJS = kernel.o malloc_wrappers.o
KERNEL_OBJS += context/child_stack.o context/context.o context/launch.o context/stack.o
KERNEL_OBJS += drivers/console.o drivers/drivers.o drivers/int_wrappers.o drivers/keyboard.o drivers/timer.o
KERNEL_OBJS += handlers/exception.o handlers/handlers.o handlers/interrupts.o handlers/panic.o
KERNEL_OBJS += lock/condvar.o lock/mutex.o lock/mutex_asm.o lock/rwlock.o
KERNEL_OBJS += prog/process.o prog/thread.o prog/thrhash.o prog/thrlist.o
KERNEL_OBJS += syscall/syscall.o syscall/syshelper.o syscall/drivers.o syscall/drivers_wrappers.o syscall/lifecycle.o syscall/lifecycle_wrappers.o syscall/management.o syscall/management_wrappers.o syscall/misc.o syscall/misc_wrappers.o syscall/paging.o syscall/paging_wrappers.o
KERNEL_OBJS += vm/frame.o vm/page.o

###########################################################################
# WARNING: Do not put **test** programs into the REQPROGS variables.  Your
#          kernel will probably not build in the test harness and you will
#          lose points.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# The shell is a really good thing to keep here.  Don't delete idle
# or init unless you are writing your own, and don't do that unless
# you have a really good reason to do so.
#
410REQPROGS = idle init shell

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended).  If you use STUDENTREQPROGS so you can
# temporarily run a special debugging version of init/idle/shell, you
# need to be very sure you blank STUDENTREQPROGS before turning your
# kernel in, or else your tweaked version will run and the test harness
# won't.
#
STUDENTREQPROGS = god
