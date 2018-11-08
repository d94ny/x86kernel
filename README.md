
# 15-410 Project 3

_Daniel Balle (dballe) & Loic Ottet (lottet)_

This is our implementation of the Pebbles kernel, named Attention les velOS, previously fatOS. Here follows a description of our design decisions relative to the various problems and challenges we faced during the creation of this massive piece of code.

## Threads and processes (in kern/prog/)

Each process is represented by a Process Control Block (PCB) containing useful metadata (see kern/inc/process.h for a complete descriptions of the fields). Processes are linked with each other by parenthood links. When a process forks a new one, the forking process becomes the parent of the forked process. The parent has the responsibility to clean up the child's data structures and allocated memory regions after it exits, via the wait() system call. If the parent has exited before some of its children, those are adopted by the init process, which serves as a garbage collector for those processes.

Each process begins with one thread (the kernel prevents a process from forking when it has more than one thread). It can create more with calls to thread_fork. All the threads of a single process share the same user memory space, but possess their own Thread Control Block (TCB) and kernel stack in the kernel memory space. The TCB contains similar information as the PCB, although different because of the different nature of the two elements.

When a process encounters a fault, meaining it has done an action which doesn't allow him to recover to a state where it is safe for it to run (reserved the case where it has a software exception handler registered, see below), it is killed by the kernel. It is then staged for garbage collection of its stack and TCB. When the thread is the last thread in its process, the PCB and other process-related resources are also freed. An exit status is transmitted to the parent when a this occurs.

Threads have the opportunity to register a software exception (hereafter swexn) handler to try and recover from faults provoked by user code. The thread can modify its state (register values) within reasonable bounds set by the kernel and the faulting instruction can then be executed again, and hopefully succeed. If a thread has no swexn handler registered, a fault caused by the thread will result in it being killed and an exit status of -2 to be set.

## Paging (in kern/vm)

Paging is handled according to the specification. The kernel is directly mapped to the physical frames with the same addresses as the virtual pages. Processes can request user space pages via the new_pages system call (and release them via remove_pages). These system calls use an underlying frame allocator to map virtual addresses to unconflicting physical addresses.

On fork operations, the entire user address space is copied from the parent process to the child. To avoid massive and useless data transfers, we implemented copy-on-write. The pages tables of both processes are simply set to point to the same frame, and the page is only copied when there is a write to one of the pages. This allows to save copies on the read-only pages, and even on read-write pages, when for example the whole paging system is wiped out on a call to exec() for example.

## Context switch (in kern/context)

Context switching is one of the most critical parts of the kernel. It allows to switch the processor state from one thread to another, with the objective that, in the resumed thread's point of view, it was never switched out in the first place.

All the context switch logic appears in a single function, context_switch(). This allows us to never explicitly set the instruction pointer during a switch, as the executed instructions on the awaken thread will be exactly the same, except in a new processor state (registers, both general purpose and special ones, in particular the paging system).

## System calls (in kern/syscall)

System calls are pieces of privileged code that the user can ask the kernel to execute for it. Most implement features described in the other sections of this README. Syscalls are called through trap gates, which allow the mode switch between user space and kernel space. Assembly wrappers complete this switch by saving the state of the user program before executing the kernel code. A very careful look is given to the user-passed arguments before they are actually used by the kernel, to avoid potential security flaws.

## Exception handlers (in kern/handlers)

Exception handlers handle all kinds of exceptions, with the exception of page faults which are handled by the VM section, as they don't necessarily mean that something went wrong somewhere. This section also contains the panic() and kernel_panic() functions, used to respectively kill a faulting thread or kill the whole kernel, if a disturbing discrepancy in the kernel state was found.

## Drivers and interrupt handlers (in kern/drivers)

The kernel supports two types of interrupts: keyboard and timer interrupts.

Keyboard interrupts are used to prompt text from the user to input in programs. The entered characters are stored in a buffer, waiting for a thread to read them and display them to the console (see below).

Timer interrupts have two functions. They are used to wake up sleeping threads when their sleeping period is over, and they implement time slicing by putting a new thread in every time an interrupt is served, thus ensuring a fair repartition of the computer resouces.

The output is represented by the console, which displays text through the print() system call, most notably. The readline() system call gets characters from the keyboard buffer (see above) and prints it to the display, while also returning the entered string to the user.

## Locking system (in kern/lock)

Locking in the kernel is enforced with three distinct objects, each one built on top of the previous ones: mutexes, condition variables and reader-writer locks. They are used through the whole kernel, according to the expected obtained behavior (see the appropriate pages for implementation details).


