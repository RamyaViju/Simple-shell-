/******************************************************************************
 *
 *  File Name........: main.c
 *
 *  Description......: Simple driver program for ush's parser
 *
 *  Author...........: Vincent W. Freeh
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
//#include <sys/unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>

#include "parse.h"
#include "main.h"

void initshell()
{
        Pipe p1;
	int fd;

        char *path = (char *) malloc((sizeof(char) * 2000));
        strcpy(path, getenv("HOME"));
        strcat(path, "/.ushrc");

        int ret = access(path, F_OK | R_OK);
        if (ret == 0)
	{
                fd = open(path, O_RDONLY);
                dup2(fd, STDIN_FILENO);
                close(fd);

                //isRcParsing = TRUE;
                //isPromptRequired = FALSE;
        }
}

static void prCmd(Cmd c)
{
        int i;
        pid_t pid;
        int status;

        if (strcmp(c->args[0], "end") == 0)
        {
	        return;
        }
	
	if (isShellCommand(c->args[0]) != -1 && built_in_fork == 0)
	{
                enable_signals();
                execute_cmd(c);
                disable_signals();
        }
	else
	{
                pid = fork();
                if (pid < 0)
                {
		        perror("Fork error!\n");
                }
		else if (pid > 0)
		{
                        // parent process
                        setpgid(pid, processGroupId);
                        int status;

                        if (isBackground == 0)
			{
                                waitpid(pid, &status, 0); // Wait for the child to complete its execution
                                if (write_to_pipe[pipeRef] == TRUE)
				{
                                        close(pipefd[pipeRef][1]);
                                        write_to_pipe[pipeRef] = FALSE;
                                        if (pipeWriteWithErr == 1)
					{
                                                pipeWriteWithErr = 0;

                                        }
                                }
                                if (WIFEXITED(status))
				{
                                        if (WEXITSTATUS(status) == 3)
					{
                                                break_pipe = 1;
                                        }
                                }
                        }
			else
			{
                                //fprintf(stdout, "[%d] %d\n", jobid, pid);
                        }

                }
		else if (pid == 0)
		{
			int whichPipeToReadFrom = pipeRef;
                        if (write_to_pipe[pipeRef] == TRUE)
			{
                                dup2(pipefd[pipeRef][1], 1);

                                if (pipeWriteWithErr == 1)
				{
                                        dup2(pipefd[pipeRef][1], 2);
                                }

                                close(pipefd[pipeRef][0]);
                                whichPipeToReadFrom = pipeRef - 1;
                        }
                        if (read_from_pipe[whichPipeToReadFrom] == TRUE)
			{
                                dup2(pipefd[whichPipeToReadFrom][0], 0);
                                close(pipefd[whichPipeToReadFrom][1]);
                        }
                        // child process

                        if (isShellCommand(c->args[0]) != -1 && built_in_fork == 1)
			{
                                enable_signals();
                                execute_cmd(c);
                                disable_signals();
                                exit(EXIT_SUCCESS);
                        }
			else
			{
                                // Only if it's not built in command
                                // fprintf(stderr, "%s I'm not built-in\n", c->args[0]);
                                enable_signals();

				/*if (strcmp(c->args[0], "cd") == 0)
				{
                			cd_cmd(c);
        			}
				else if (strcmp(c->args[0], "where") == 0)
				{
                			where_cmd(c);
        			}
				else if (strcmp(c->args[0], "setenv") == 0)
				{
                			setenv_cmd(c);
        			}
				else if (strcmp(c->args[0], "unsetenv") == 0)
				{
                			unsetenv_cmd(c);
        			}
				else if (strcmp(c->args[0], "nice") == 0)
				{
                			nice_cmd(c);
				}
				else if (strcmp(c->args[0], "logout") == 0)
                                {
                                        //logout_cmd(c);
                                        exit(EXIT_SUCCESS);
                                }
				else
				{*/
                                execvp(c->args[0], c->args);

                                // Shouldn't return. If returns, them some error occured
                                switch (errno)
				{
                                	case EISDIR:
                                	case EACCES:
                                        	fprintf(stderr, "permission denied\n");
                                        	killpg(processGroupId, SIGKILL);
                                        	break;
					case ENOENT:
						fprintf(stderr, "command not found\n");
						killpg(processGroupId, SIGKILL);
						break;
					default:
						break;
                                }
				//}
			}

                        exit(3);
                }

        }

}

static prSymbols(Cmd c)
{
	int i;
        int fd;

	if ( c )
	{
		//printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);

		if (c->exec == Tamp)
		{
                        isBackground = 1;
                        //jobid++;
                }
		else
		{
                        isBackground = 0;
                }

		if ( c->in == Tin )
		{
			//printf("<(%s) ", c->infile);
			fd = open(c->infile, O_RDONLY);
                        dup2(fd, STDIN_FILENO);
                        close(fd);
		}

		if (c->in == Tpipe || c->in == TpipeErr)
		{
                        read_from_pipe[pipeRef] = 1;
                }

		if ( c->out != Tnil )
			switch ( c->out )
			{
				case Tout:
					//printf(">(%s) ", c->outfile);
					fd = open(c->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
                                	dup2(fd, STDOUT_FILENO);
                                	close(fd);
                                	restore = 1;
					break;
				case Tapp:
					//printf(">>(%s) ", c->outfile);
					fd = open(c->outfile, O_CREAT | O_WRONLY | O_APPEND, 0777);
                                	dup2(fd, STDOUT_FILENO);
                                	close(fd);
                                	restore = 1;
					break;
				case ToutErr:
					//printf(">&(%s) ", c->outfile);
					fd = open(c->outfile, O_CREAT | O_WRONLY | O_TRUNC, 0777);
                                	dup2(fd, STDOUT_FILENO);
                                	dup2(fd, STDERR_FILENO);
                                	close(fd);
                                	restore = 1;
					break;
				case TappErr:
					//printf(">>&(%s) ", c->outfile);
					fd = open(c->outfile, O_CREAT | O_WRONLY | O_APPEND, 0777);
                                	dup2(fd, STDOUT_FILENO);
                                	dup2(fd, STDERR_FILENO);
                                	close(fd);
                                	restore = 1;
					break;
				case Tpipe:
					//printf("| ");
					pipeRef++;
                                	pipe(pipefd[pipeRef]);
                                	write_to_pipe[pipeRef] = 1;
                                	restore = 0;
					break;
				case TpipeErr:
					//printf("|& ");
					pipeRef++;
                                	pipe(pipefd[pipeRef]);
                                	write_to_pipe[pipeRef] = 1;
                                	pipeWriteWithErr = 1;
                                	restore = 0;
					break;
				default:
					fprintf(stderr, "Shouldn't get here\n");
					exit(-1);
			}

		/*if ( c->nargs > 1 )
		{
			printf("[");
			for ( i = 1; c->args[i] != NULL; i++ )
				printf("%d:%s,", i, c->args[i]);
			
			printf("\b]");
		}
		putchar('\n');*/
		// this driver understands one command
		if ( !strcmp(c->args[0], "end") )
			exit(0);
	}
}

static void prPipe(Pipe p)
{
	int i = 0;
	Cmd c;

	processGroupId++;

	if ( p == NULL )
		return;

	//printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
	
	for ( c = p->head; c != NULL && break_pipe == 0; c = c->next )
	{
		//printf("  Cmd #%d: ", ++i);
		built_in_fork = (c->next != NULL) ? 1 : 0;
		prSymbols(c);
		prCmd(c);

		if (c->next == NULL)
		{
                        restore = 1;
                }

                if (restore == 1)
		{
                        restoreFD();
                }
	}
	break_pipe = 0;
	//printf("End pipe\n");
	prPipe(p->next);
}

int main(int argc, char *argv[])
{
	Pipe p;
	//char *host = "armadillo";

	int num;

	saveFD();

	num = ftell(stdin);
	if (num != -1)
                isStdinPresent = TRUE;
        initshell();

	while ( 1 )
	{
		printf("%s%% ", host);
		disable_signals();
		pipeRef = -1;

		p = parse();
		prPipe(p);
		freePipe(p);
	}
}

void cd_cmd(Cmd c)
{
        char destination_path[256];
        if (c->args[1] == NULL || c->args[1][0] == '~')
	{
                strcpy(destination_path, getenv("HOME"));
        }
	else
	{
                strcpy(destination_path, c->args[1]);
        }

	//printf("\nDestination path is %s\n", destination_path);

	/*char buffer[256];
	if (getcwd(buffer, sizeof buffer) >= 0)
	{
    		printf("Old wd: %s\n", buffer);
	}*/

        if (chdir(destination_path) == -1)
	{
                switch (errno)
		{
                	case EACCES:
                        	fprintf(stderr, "Permission Denied!\n");
                        	break;
			case ENOTDIR:
				fprintf(stderr, "Not a directory!\n");
				break;
			case ENOENT:
				fprintf(stderr, "Directory does not exist!\n");
				break;
                }
        }

	/*if (getcwd(buffer, sizeof buffer) >= 0)
	{
    		printf("New wd: %s\n", buffer);
	}*/
}

void where_cmd(Cmd c)
{
        char *real_path = getenv("PATH");
        char path[1000];
        strcpy(path, real_path);
        char delimiter[1];
        delimiter[0] = ':'; // The path is separated using :
        /*if (isBuiltIn(c->args[1]) != -1) {
                printf("%s: shell built-in command.\n", c->args[1]);
        }*/
        char *p = strtok(path, delimiter);
        char *pathToCommand = (char *) malloc(sizeof(char) * 150);

        while (p != NULL) {
                strcpy(pathToCommand, p); // Need to generate <path>/<command>
                strcat(pathToCommand, "/");
                strcat(pathToCommand, c->args[1]);
                //if (checkIsCommandPath(pathToCommand))
                
		struct stat sb;
		if (stat(pathToCommand, &sb) == 0 && S_ISREG(sb.st_mode))
		{
			printf("%s\n", pathToCommand);
		}
                p = strtok(NULL, delimiter);
        }

}

void pwd_cmd()
{
        char *cwd = (char *) malloc(2000 * sizeof(char));
        cwd = get_current_dir_name();
        fprintf(stdout, "%s\n", cwd);
}

void echo_cmd(Cmd c)
{
        int i;
        if (c->args[1] == NULL)
	{
                fprintf(stdout, "\n");
                return;
        }
        for (i = 1; c->args[i] != NULL; i++)
	{
                fprintf(stdout, "%s", c->args[i]);

                if (c->args[i + 1] != NULL)
                {
		        fprintf(stdout, " ");
        	}
	}
        fprintf(stdout, "\n");
}

void setenv_cmd(Cmd c)
{
        extern char **environ;
        char **available_env = environ;

        if (c->args[1] == NULL)
	{
                // If there are no arguments, Print all available paths
                for (; *available_env != NULL; *available_env++)
                {
		        fprintf(stdout, "%s\n", *available_env);
		}
        }
	else
	{
                // Some arguments are there.
                // args[1] = NAME,
                // args[2] = VALUE
                if (c->args[2] == NULL)
		{
                        // If no word
                        setenv(c->args[1], "", 1);
                }
		else
		{
                        setenv(c->args[1], c->args[2], 1);
                }
        }
}

void unsetenv_cmd(Cmd c)
{
        if (c->args[1] != NULL)
	{
                unsetenv(c->args[1]);
        }
	else
	{
                fprintf(stderr, "unsetenv: Too few arguments.");
        }
}

void nice_cmd(Cmd c)
{
        int which, who, prio = 4;
        pid_t pid;
        which = PRIO_PROCESS;
        
	if (c->args[1] == NULL)
	{
                who = getpid();
                setpriority(which, who, prio);
        }
	else
	{
                prio = atoi(c->args[1]);
                // printf("The priority: %d\n", prio);
                if (c->args[2] == NULL)
		{
                        who = getpid();
                        setpriority(which, who, prio);

                }
		else
		{
                        Cmd tempCmd = (Cmd) malloc(sizeof(Cmd) * sizeof(c));
                        tempCmd->args = malloc(sizeof(c->args) * sizeof(char *));
                        int i;
                        i = 0;
                        
			while (c->args[i + 2])
			{
                                tempCmd->args[i] = (char *) malloc(sizeof(c->args[i + 2]) * sizeof(char));
                                strcpy(tempCmd->args[i], c->args[i + 2]);
                                i++;
                        }

                        tempCmd->args[i] = NULL;
                        if (isShellCommand(tempCmd->args[0]) != -1)
			{
                                who = getpid();
                                setpriority(which, who, prio);
                                
				if (strcmp(tempCmd->args[0], "cd") == 0)
				{
                                        cd_cmd(tempCmd);
                                }
				else if (strcmp(tempCmd->args[0], "logout") == 0)
				{
                                        exit(0);
                                }
				else if (strcmp(c->args[0], "pwd") == 0)
				{
                			pwd_cmd();
        			}
                                else if (strcmp(tempCmd->args[0], "where") == 0)
				{
                                        where_cmd(tempCmd);
                                }
				else if (strcmp(tempCmd->args[0], "setenv") == 0)
				{
                                        setenv_cmd(tempCmd);
                                }
				else if (strcmp(tempCmd->args[0], "unsetenv") == 0)
				{
                                        unsetenv_cmd(tempCmd);
                                }
				else if (strcmp(tempCmd->args[0], "echo") == 0)
				{
                                        echo_cmd(tempCmd);
                                }
                        }
			else
			{
                                who = fork();
                                setpriority(which, who, prio);
                                
				if (who == 0)
				{
                                        execvp(tempCmd->args[0], tempCmd->args);
                                }
				else if (who > 0)
				{
                                        wait(NULL);
                                }
                        }
                }
        }

}

int isShellCommand(char *c)
{
        int i = 0;
        for (i = 0; i < 12; i++) {
                if (strcmp(shell_commands[i], c) == 0)
                        return i;
        }
        return -1;
}

static void execute_cmd(Cmd c)
{
        if (strcmp(c->args[0], "cd") == 0)
	{
                cd_cmd(c);
        } 
        else if (strcmp(c->args[0], "logout") == 0)
	{
                exit(0);
        }
        else if (strcmp(c->args[0], "where") == 0)
	{
                where_cmd(c);
        }
	else if (strcmp(c->args[0], "pwd") == 0)
	{
                pwd_cmd();
        }
	else if (strcmp(c->args[0], "setenv") == 0)
	{
                setenv_cmd(c);
        }
	else if (strcmp(c->args[0], "unsetenv") == 0)
	{
                unsetenv_cmd(c);
        }
	else if (strcmp(c->args[0], "nice") == 0)
	{
                nice_cmd(c);
        }
	else if (strcmp(c->args[0], "echo") == 0)
	{
        	echo_cmd(c);
        }
}

/*........................ end of main.c ....................................*/
