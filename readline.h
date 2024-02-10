#ifndef READLINE_H
#define READLINE_H

char *read_canonical_line(const char *prompt);
char *read_non_canonical_line(const char *prompt);
void delete_char(void);
void printCmd(const char *aux, char *buf);
void cleanStdOut(char *buffer, int current_pos);
void insert_and_move_positions(char *buffer, char c, int current_pos);
void printNewCharacters(char *buffer, int current_pos);

#endif  // READLINE_H
