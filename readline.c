#include "defs.h"
#include "readline.h"
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include "history.h"

#define backspace "\b \b"

#define CHAR_NL '\n'
#define CHAR_EOF '\004'
#define CHAR_BACK '\b'
#define CHAR_DEL 127
#define CHAR_ESC '\033'

static char buffer[BUFLEN];

/*
 * genera una secuencia de bytes
 * que indican que se debe borrar un byte
 */
void
delete_char()
{
	assert(write(STDOUT_FILENO, backspace, 3) > 0);
}

/*
 * Lee de la salida estandar y guarda los valores en un buffer.
 * (Utilización exclusiva para los tests).
 */
char *
read_canonical_line(const char *prompt)
{
	int i = 0, c = 0;

#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif

	memset(buffer, 0, BUFLEN);

	c = getchar();

	while (c != END_LINE && c != EOF) {
		buffer[i++] = c;
		c = getchar();
	}

	// if the user press ctrl+D
	// just exit normally
	if (c == EOF)
		return NULL;

	buffer[i] = END_STRING;

	return buffer;
}

void
printCmd(const char *aux, char *buf)
{
	char c;
	for (size_t i = 0; i < strlen(aux); i++) {
		c = aux[i];
		assert(write(STDOUT_FILENO, &c, 1) > 0);
		buf[i] = c;
	}
	buf[strlen(buf)] = '\0';
}

void
cleanStdOut(char *buffer, int current_pos)
{
	for (size_t i = current_pos; i < strlen(buffer); i++) {
		assert(write(STDOUT_FILENO, " ", 1) > 0);
	}
	for (size_t i = 0; i < strlen(buffer); i++) {
		delete_char();
	}
	for (int i = 0; i < BUFLEN; i++) {
		buffer[i] = '\0';
	}
}
/*
 * c: caracter que se quiere introducir
 * current_pos: posición donde se quiere ingresar el caracter
 * buffer: vector de caracteres que contiene toda la linea de comandos.
 *
 * Se realiza un desplazamiento de todos los caracteres en una posición a la
 * derecha y se inserta el nuevo caracter.
 * Se imprime el contenido del buffer y deja el cursor en la posición esperada.
 */
void
insert_and_move_positions(char *buffer, char c, int current_pos)
{
	int size = strlen(buffer);
	for (int i = size - 1; i >= current_pos - 1; i--) {
		buffer[i + 1] = buffer[i];
	}
	for (int i = current_pos; i < size; i++) {
		assert(write(STDOUT_FILENO, &buffer[i + 1], 1) > 0);
	}

	for (int i = 0; i < size - current_pos; i++) {
		assert(write(STDOUT_FILENO, "\b", 1) > 0);
	}

	buffer[current_pos] = c;
}

/*
 * Se recibe el buffer con la linea completa y la posición actual del cursor.
 * Imprime los caracteres nuevamente desde el cursor hasta el final de la linea
 * y dejando al cursor en la posición donde se encontraba.
 */
void
printNewCharacters(char *buffer, int current_pos)
{
	int size = strlen(buffer);

	for (int i = current_pos; i <= size - 1; i++) {
		buffer[i] = buffer[i + 1];
	}

	for (int i = current_pos; i < size; i++) {
		assert(write(STDOUT_FILENO, &buffer[i], 1) > 0);
	}

	assert(write(STDOUT_FILENO, " ", 1) > 0);
	assert(write(STDOUT_FILENO, "\b", 1) > 0);

	for (int i = 0; i < size - current_pos - 1; i++) {
		assert(write(STDOUT_FILENO, "\b", 1) > 0);
	}
}
/*
 * Lee de la entrada estandar los caracteres y llena el buffer con la linea
 * escrita. Devuelve el comando a ejecutar.
 *
 * Flecha arriba/abajo: Se reemplazará el contenido del buffer
 * con un comando realizado anteiormente.
 *
 * Flecha izquierda/derecha: Se moverá el cursor del buffer para la capacidad de
 * escribir cualquier parte del buffer, no exclusivamente al final.
 *
 * Ctrl + flecha izquierda/derehca: El cursor se desplazará hacia el fin o
 * principio de una nueva palabra.
 *
 * tecla HOME / END : El cursor se moverá hacia el principio (tecla HOME)
 * o el final (tecla END) de la linea.
 */

char *
read_non_canonical_line(const char *prompt)
{
#ifndef SHELL_NO_INTERACTIVE
	fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
	fprintf(stdout, "%s", "$ ");
#endif
	char c;
	int line_pos = 0;
	int current_pos = 0;
	char buffer[BUFLEN];
	memset(buffer, 0, BUFLEN);
	while (true) {
		assert(read(STDIN_FILENO, &c, 1) > 0);


		if (c == CHAR_NL) {
			// tecla "enter"
			buffer[line_pos] = '\0';
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			return strdup(buffer);
		}

		if (c == CHAR_EOF) {
			// teclas "Ctrl-D"
			return NULL;
		}

		if (c == CHAR_DEL) {
			// tecla "Backspace"
			if (line_pos < 1 || current_pos < 1) {
				// estamos al comienzo de la pantalla
				continue;
			}
			delete_char();
			current_pos--;
			printNewCharacters(buffer, current_pos);
			buffer[line_pos--] = '\0';
		}

		if (c == CHAR_ESC) {
			// comienzo de una sequencia
			// de escape
			char esc_seq;
			assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

			if (esc_seq != '[')
				continue;

			assert(read(STDIN_FILENO, &esc_seq, 1) > 0);
			if (esc_seq == 'A') {
				// Flecha Arriba

				char *prevCmd = getPreviousCmd();
				cleanStdOut(buffer, current_pos);
				printCmd(prevCmd, buffer);
				line_pos = strlen(prevCmd);
				current_pos = line_pos;
			}
			if (esc_seq == 'B') {
				// Flecha Abajo
				char *nextCmd = getNextCmd();
				cleanStdOut(buffer, current_pos);
				printCmd(nextCmd, buffer);
				line_pos = strlen(nextCmd);
				current_pos = line_pos;
			}
			if (esc_seq == 'C') {
				// Flecha derecha
				if (current_pos < line_pos) {
					char c = buffer[current_pos];  // Posicion donde esta el prompt
					assert(write(STDOUT_FILENO, &c, 1) > 0);
					current_pos++;
				}
			}
			if (esc_seq == 'D') {
				// Flecha izquierda
				if (current_pos > 0) {
					assert(write(STDOUT_FILENO, "\b", 1) > 0);
					current_pos--;
				}
			}
			if (esc_seq == 'H') {
				// Home
				while (current_pos > 0) {
					assert(write(STDOUT_FILENO, "\b", 1) > 0);
					current_pos--;
				}
			}
			if (esc_seq == 'F') {
				// EndFile
				while (current_pos < line_pos) {
					char c = buffer[current_pos];  // Posicion donde esta el prompt
					assert(write(STDOUT_FILENO, &c, 1) > 0);
					current_pos++;
				}
			}
			if (esc_seq == '1') {
				// Lee el ;
				assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

				if (esc_seq != ';') {
					continue;
				}

				// Leo el 5
				assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

				if (esc_seq != '5') {
					continue;
				}

				// Leo C o D
				assert(read(STDIN_FILENO, &esc_seq, 1) > 0);

				if (esc_seq == 'C') {
					// Ctrl + Flecha Derecha
					if (current_pos < line_pos) {
						char c = buffer[current_pos];  // Posicion donde esta el prompt
						assert(write(STDOUT_FILENO, &c, 1) >
						       0);
						current_pos++;
					}
					while (current_pos < line_pos &&
					       buffer[current_pos] != ' ') {
						char c = buffer[current_pos];  // Posicion donde esta el prompt
						assert(write(STDOUT_FILENO, &c, 1) >
						       0);
						current_pos++;
					}
				}

				if (esc_seq == 'D') {
					// Ctrl + Flecha Izquierda
					if (current_pos > 0) {
						assert(write(STDOUT_FILENO,
						             "\b",
						             1) > 0);
						current_pos--;
					}
					while (current_pos > 0 &&
					       buffer[current_pos - 1] != ' ') {
						assert(write(STDOUT_FILENO,
						             "\b",
						             1) > 0);
						current_pos--;
					}
				}
			}
		}
		if (isprint(c)) {  // si es visible
			assert(write(STDOUT_FILENO, &c, 1) > 0);
			insert_and_move_positions(buffer, c, current_pos);
			current_pos++;
			line_pos++;
		}
	}
}
