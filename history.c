#include "history.h"
#include "defs.h"
#include "runcmd.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_HISTORY 100

// Para el caso en que file_len = MAX_HISTORY - POCOS quedaria espacio para POCOS comando
#define MIN_SPACE 5

char history_list[MAX_HISTORY][BUFLEN];
int history_count;  // cantidad total de comandos de la lista
int current_cmd;    // indice del cmd actual

/*
 * Devuelve el path del archivo del history
 */
char *
getFilePath()
{
	char *filename = getenv("HISTFILE");
	char *homeName = getenv("HOME");
	char aux[BUFLEN];
	if (isatty(STDIN_FILENO)) {
		strcpy(aux, homeName);
		if (filename == NULL) {
			strcat(aux, "/.fisop_history");
			filename = aux;
		}
		return strdup(filename);
	}
	return NULL;
}


/* History: Archivo de lectura.
 * n: Cantidad de lineas que se desean leer.
 * fileLines: Cantidad total de lineas que posee el archivo de lectura.
 *
 * Imprime por pantalla las últimas n lineas del archivo de lectura.
 */
void
printHistory(FILE *history, int n, int fileLines)
{
	int firstPosition = fileLines - n;
	int currentLine = 0;
	char *line = NULL;
	size_t len = 0;

	while (getline(&line, &len, history) != -1) {
		currentLine++;
		if (currentLine > firstPosition) {
			printf_debug("%s", line);
		}
	}
	free(line);
}


/*
 * Devuelve la cantidad de lineas que posee el archivo .fisop_history.
 */
int
getFileLen(FILE *history)
{
	int lenFile = 0;
	char *line = NULL;
	size_t len = 0;
	while (getline(&line, &len, history) != -1) {
		lenFile++;
	}
	free(line);
	return lenFile;
}

/*
 * Inicializa la lista de comandos a partir del archivo de historial
 */

void
loadHistory()
{
	if (isatty(STDIN_FILENO)) {
		char *path = getFilePath();
		current_cmd = 0;
		history_count = 0;
		FILE *file = fopen(path, "a+");

		int len_file = getFileLen(file);

		rewind(file);
		int begin_idx = 0;
		if (len_file > MAX_HISTORY - MIN_SPACE) {
			begin_idx = len_file - MAX_HISTORY / 2;
		}
		char *line = NULL;
		int i = 0;
		size_t len = 0;

		while (getline(&line, &len, file) != -1) {
			if (i >= begin_idx) {
				line[strlen(line) - 1] = '\0';
				strcpy(history_list[i], line);
			}
			i++;
		}
		history_count = i;
		current_cmd = i;
		free(line);
		fclose(file);
		free(path);
	}
}

/*
 * n: cantidad de argumentos que se desean leer (ALL = -1)
 * Lee el archivo por primera vez y cuenta la cantidad de lineas, luego
 * se llama a printHistory para imprimirlos por pantalla
 */
void
readFile(const int n)
{
	char *path = getFilePath();
	FILE *file = fopen(path, "r");
	int len = getFileLen(file);

	rewind(file);
	if (n >= 0) {
		printHistory(file, n, len);
	} else {
		printHistory(file, len, len);
	}
	fclose(file);
	free(path);
}

/*
 * Guarda el comando cmd en la lista del history y tambien en el archivo
 */

void
saveLineHistory(char *cmd)
{
	char *path = getFilePath();
	if (path) {
		FILE *history;
		history = fopen(path, "a+");
		if (history == 0) {
			printf_debug("error file: %s\n", path);
		}
		// if(isprint(*cmd)) {
		fprintf(history, "%s\n", cmd);
		//}
		fclose(history);
	}
	strcpy(history_list[history_count++], cmd);
	current_cmd = history_count;
	free(path);
}

/*
 * Devuelve el comando anterior ejecutado respecto del current_cmd
 */

char *
getPreviousCmd()
{
	if (current_cmd > 0) {
		return history_list[--current_cmd];
	}
	return history_list[current_cmd];
}

/*
 * Devuelve el comando siguiente ejecutado respecto del current_cmd
 */

char *
getNextCmd()
{
	if (current_cmd < history_count) {
		return history_list[++current_cmd];
	}
	return history_list[current_cmd];
}