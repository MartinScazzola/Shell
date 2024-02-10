#ifndef EXEC_H
#define EXEC_H

#include "defs.h"
#include "types.h"
#include "utils.h"
#include "freecmd.h"

extern struct cmd *parsed_pipe;

void exec_cmd(struct cmd *c);
void redirectByError(struct execcmd *cmd);
void redirectByStdOut(struct execcmd *cmd);
void redirectFD(struct execcmd *cmd);
void handle_pipe_cmd(struct pipecmd *p);
void verifyEArgs(struct execcmd *command);
void redirectByStdIn(struct execcmd *cmd);

#endif  // EXEC_H
