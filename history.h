#define MAX_HISTORY 100
#include <stdio.h>

void printHistory(FILE *history, int n, int fileLines);

int getFileLen(FILE *history);


char *getFilePath(void);
char *getPreviousCmd(void);
char *getNextCmd(void);

void loadHistory(void);
void readFile(const int n);
void saveLineHistory(char *cmd);