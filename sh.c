#include "defs.h"
#include "readline.h"
#include "runcmd.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include "history.h"

struct termios saved_attributes;

// Function Prototypes
void reset_input_mode(void);
void set_input_mode(void);
static char *readCanonicalMode();
static char *readNonCanonicalMode();


void
reset_input_mode(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode(void)
{
	struct termios tattr;

	/* Make sure stdin is a terminal. */
	if (!isatty(STDIN_FILENO)) {
		fprintf(stderr, "Not a terminal.\n");
		exit(EXIT_FAILURE);
	}

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr(STDIN_FILENO, &saved_attributes);

	/* Set the funny terminal modes. */
	tcgetattr(STDIN_FILENO, &tattr);
	/* Clear ICANON and ECHO. We'll do a manual echo! */
	tattr.c_lflag &= ~(ICANON | ECHO);
	/* Read one char at a time */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

char prompt[PRMTLEN] = { 0 };

// runs a shell command

static char *
readCanonicalMode()
{
	char *cmd;
	cmd = read_canonical_line(prompt);
	return cmd;
}

static char *
readNonCanonicalMode()
{
	char *cmd;
	set_input_mode();
	cmd = read_non_canonical_line(prompt);
	reset_input_mode();
	return cmd;
}

static void
run_shell()
{
	loadHistory();

	char *cmd;
	while (true) {
		if (!isatty(STDIN_FILENO)) {
			cmd = readCanonicalMode();
		} else {
			cmd = readNonCanonicalMode();
		}

		if (cmd == NULL) {
			return;
		}
		if (run_cmd(cmd) == EXIT_SHELL) {
			return;
		}
		// free(cmd);
	}
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}
}

int
main(void)
{
	init_shell();

	run_shell();

	return 0;
}
