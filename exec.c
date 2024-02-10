#include "exec.h"

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	for (int i = 0; i < eargc; i++) {
		int index = block_contains(eargv[i], '=');
		if (index > 0) {
			char key[BUFLEN];
			get_environ_key(eargv[i], key);

			char value[BUFLEN];
			get_environ_value(eargv[i], value, index);

			setenv(key, value, 1);
		}
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int openingResult;

	if (flags & O_CREAT) {
		openingResult = open(file, flags, S_IWUSR | S_IRUSR);
	} else {
		openingResult = open(file, flags);
	}


	return openingResult;
}

void
redirectByError(struct execcmd *cmd)
{
	if (strcmp(cmd->err_file, "&1") == 0) {
		// Redirecciona al output
		int dup2Result = dup2(STDOUT_FILENO, STDERR_FILENO);
		if (dup2Result == ERROR_ID) {
			exit(ERROR_ID);
		}
	} else {
		int newFd = open_redir_fd(cmd->err_file, O_CREAT | O_RDWR);
		if (newFd < 0) {
			exit(ERROR_ID);
		}

		int dup2Result = dup2(newFd, STDERR_FILENO);
		close(newFd);
		if (dup2Result == ERROR_ID) {
			exit(ERROR_ID);
		}
	}
}


void
redirectByStdIn(struct execcmd *cmd)
{
	int newFd = open_redir_fd(cmd->in_file, O_RDONLY);
	if (newFd < 0) {
		exit(ERROR_ID);
	}

	int dup2Result = dup2(newFd, STDIN_FILENO);
	close(newFd);
	if (dup2Result == ERROR_ID) {
		exit(ERROR_ID);
	}
}

void
redirectByStdOut(struct execcmd *cmd)
{
	int newFd = open_redir_fd(cmd->out_file, O_CREAT | O_RDWR | O_TRUNC);
	if (newFd < 0) {
		exit(ERROR_ID);
	}

	int dup2Result = dup2(newFd, STDOUT_FILENO);
	close(newFd);
	if (dup2Result == ERROR_ID) {
		exit(ERROR_ID);
	}
}

void
redirectFD(struct execcmd *cmd)
{
	if (strlen(cmd->out_file) > 0) {
		redirectByStdOut(cmd);
	}
	if (strlen(cmd->err_file) > 0) {
		redirectByError(cmd);
	}
	if (strlen(cmd->in_file) > 0) {
		redirectByStdIn(cmd);
	}
}

void
handle_pipe_cmd(struct pipecmd *p)
{
	int pipe_fds[2];
	if (pipe(pipe_fds) < 0) {
		printf_debug("Error al crear pipe\n");
		exit(ERROR_ID);
	}

	pid_t pid_left = fork();
	if (pid_left < 0) {
		close(pipe_fds[READ]);
		close(pipe_fds[WRITE]);
		printf_debug("Error al hacer fork\n");
		exit(ERROR_ID);
	}

	if (pid_left == 0) {
		close(pipe_fds[READ]);
		int result = dup2(pipe_fds[WRITE], STDOUT_FILENO);
		if (result == ERROR_ID) {
			close(pipe_fds[WRITE]);
			exit(ERROR_ID);
		}
		close(pipe_fds[WRITE]);
		exec_cmd(p->leftcmd);
	} else {
		close(pipe_fds[WRITE]);
		pid_t pid_right = fork();
		if (pid_right < 0) {
			waitpid(pid_left, NULL, 0);
			close(pipe_fds[READ]);
			printf_debug("Error al hacer fork\n");
			exit(ERROR_ID);
		}
		if (pid_right == 0) {
			int result = dup2(pipe_fds[READ], STDIN_FILENO);
			if (result == ERROR_ID) {
				close(pipe_fds[READ]);
				exit(ERROR_ID);
			}
			close(pipe_fds[READ]);
			exec_cmd(p->rightcmd);
		} else {
			close(pipe_fds[READ]);
			waitpid(pid_left, NULL, 0);
			waitpid(pid_right, NULL, 0);
		}
	}
}


void
verifyEArgs(struct execcmd *command)
{
	for (int i = 0; i < command->eargc; i++) {
		if (command->eargv[i] == NULL) {
			command->eargv[i] = '\0';
		}
	}
	for (int i = 0; i < command->argc; i++) {
		if (command->argv[i] == NULL) {
			command->argv[i] = '\0';
		}
	}
}

// executes a command - does not return
void
exec_cmd(struct cmd *cmd)
{
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipecmd *p;

	switch (cmd->type) {
	case EXEC:
		// spawns a command
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		int result = execvp(e->argv[0], e->argv);

		if (result == ERROR_ID) {
			free_command(cmd);
			printf("Error en execvp\n");
			exit(ERROR_ID);
		}
		break;


	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;

		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		//
		// To check if a redirection has to be performed
		// verify if file name's length (in the execcmd struct)
		// is greater than zero
		r = (struct execcmd *) cmd;
		redirectFD(r);
		r->type = EXEC;
		exec_cmd((struct cmd *) r);

		break;
	}

	case PIPE: {
		p = (struct pipecmd *) cmd;
		handle_pipe_cmd(p);
		free_command(parsed_pipe);
		exit(0);
	}
	}
}
