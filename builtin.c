#include "builtin.h"
#define ALL -1
#include "history.h"
// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	if (strcmp(cmd, "exit") == 0) {
		return true;
	} else {
		return false;
	}
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	char dir[BUFLEN];
	if (strcmp("cd", cmd) == 0 || strcmp("cd ", cmd) == 0) {
		char *home = getenv("HOME");
		strcpy(dir, home);

		int ch_result = chdir(dir);

		if (ch_result == -1) {
			return false;
		}

	} else if (strncmp("cd ", cmd, 3) == 0) {
		char dir[BUFLEN];
		int ch_result = chdir(cmd + 3);

		if (ch_result == -1) {
			return false;
		}

		char *pointer = getcwd(dir, PRMTLEN);
		if (pointer == NULL) {
			printf("Error getting the pathname to working "
			       "directory\n");
			exit(-1);
		}
	} else {
		return false;
	}
	sprintf(prompt, "(%s)", dir);
	return true;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char path[BUFLEN];
		char *pointer = getcwd(path, BUFLEN);

		if (pointer == NULL) {
			printf("Error getting the pathname to working "
			       "directory\n");
			exit(-1);
		}

		printf("%s\n", path);
		return true;
	}
	return false;
}

/* si el comando no posee argumentos se mostrará por pantalla
 * el historial completo, de lo contrario se mostrará la cantidad de
 * comandos igual al argumento.
 *
 *  Caso 1: history
 *  Caso 2: history 7
 *
 *  Retorna true si la función history fue invocada correctamente.
 */

int
history(char *cmd)
{
	if (strncmp(cmd, "history", 7) == 0) {
		int len = strlen(cmd);
		if (len > 7) {
			int n = atoi(cmd + 8);
			readFile(n);
		} else {
			readFile(ALL);
		}
		return true;
	}

	return false;
}
