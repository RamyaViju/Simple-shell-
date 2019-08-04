/*#########################################################################################
 *      This is a header file that contains certain functions used in the 'main.c' file.
 *      Some code used here is picked up from various online resources.
 *      Please refer the REFERENCES file for the list of online resources used.
 *
 *      --------------------------------------
 *      Author: Ramya Vijayakumar
 *      --------------------------------------
 *#########################################################################################
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "parse.h"

#define TRUE 1
#define FALSE 0

char *host = "armadillo";

int ofd_stdin;
int ofd_stdout;
int ofd_stderr;

int isStdinPresent = FALSE;

void saveFD()
{
        ofd_stdin = dup(STDIN_FILENO);
        ofd_stdout = dup(STDOUT_FILENO);
        ofd_stderr = dup(STDERR_FILENO);
}

void restoreFD()
{
        dup2(ofd_stdin, STDIN_FILENO);
        dup2(ofd_stdout, STDOUT_FILENO);
        dup2(ofd_stderr, STDERR_FILENO);
}

int pipeRef = -1;

void signal_handler()
{
        fprintf(stdout, "\n");
        char *host = "armadillo";
	printf("%s%% ", host);
        fflush(stdout);
}

void disable_signals()
{
        /* Ignore interactive and job-control signals.  */
        signal(SIGINT, signal_handler);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, signal_handler); // handle ctrl+d
        signal(SIGTSTP, SIG_IGN); // handle ctrl+z
}

void enable_signals()
{
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTERM, SIG_DFL); // handle ctrl+d
        signal(SIGTSTP, SIG_DFL); // handle ctrl+z
}

pid_t processGroupId = 7171;
int break_pipe = 0;
int built_in_fork;
int restore;
int isBackground = 0;
int write_to_pipe[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int pipefd[100][2];
int pipeWriteWithErr = 0;
int read_from_pipe[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static char *shell_commands[] = {
                "bg", "cd", "fg", "echo", "jobs", "kill", "logout",
                "nice", "pwd", "setenv", "unsetenv", "where",
};

static void execute_cmd(Cmd c);

int isBuiltIn(char *c);
void cd_cmd(Cmd c);
void where_cmd(Cmd c);
void pwd_cmd();
void setenv_cmd(Cmd c);
void unsetenv_cmd(Cmd c);
void nice_cmd(Cmd c);
void echo_cmd(Cmd c);
//void logout_cmd(Cmd c);
