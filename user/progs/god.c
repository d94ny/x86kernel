#include <stdlib.h>
#include <simics.h>
#include <syscall.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

void splash_screen() {
    set_cursor_pos(0,0);

    printf("\n");

    set_term_color(BGND_RED);
    printf("                                       ");
    set_term_color(BGND_LGRAY); printf("  ");
    set_term_color(BGND_RED);
    printf("                                       ");

    set_term_color(BGND_RED);
    printf("                                     ");
    set_term_color(BGND_LGRAY); printf("      ");
    set_term_color(BGND_RED);
    printf("                                     ");
    
    set_term_color(BGND_RED);
    printf("                                       ");
    set_term_color(BGND_LGRAY); printf("  ");
    set_term_color(BGND_RED);
    printf("                                       ");

    set_term_color(BGND_RED | FGND_WHITE);
    printf("\
                          _   _          \
   _   _                               \
                     /\\  | | | |        \
   | | (_)                              \
                    /  \\ | |_| |_ ___ _ _\
_ | |_ _  ___  _ __                    \
                   / /\\ \\| __| __/ _ \\ \
'_ \\| __| |/ _ \\| '_ \\                   \
                  / ____ \\ |_| ||  __/ | \
| | |_| | (_) | | | |                  \
                 /_/    \\_\\__|\\__\\___|\
_| |_|\\__|_|\\___/|_| |_|                  \
                 | |                     | \
|/ __ \\ / ____|                      \
                 | | ___  ___  __   _____| \
| |  | | (___                        \
                 | |/ _ \\/ __| \\ \\ / / \
_ \\ | |  | |\\___ \\                       \
                 | |  __/\\__ \\  \\ V /  \
__/ | |__| |____) |                      \
                 |_|\\___||___/   \\_/ \\__\
_|_|\\____/|_____/                       \
                                            \
                                    ");

    printf("\
                           \
Riding on the fatOS kernel\
                           ");
}

int main(int argc, char **argv) {

	/* IDLE */
	int tid = fork();
	if (tid == 0) {
		exec("idle", NULL);
	}

	splash_screen();

	/* INIT */
	char *program = "init";
	char *args[2] = {program, NULL};
	exec(program, args);

	halt();

	// We should NEVER get here
	return -1;
}
