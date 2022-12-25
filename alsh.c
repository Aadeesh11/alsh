#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFF_SIZE 256
int interactive = 0;
char *paths[BUFF_SIZE] = {"/bin", NULL};
FILE *in;
char *line = NULL;

void printError()
{
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

void clean(void)
{
    if (line != NULL)
        free(line);

    // todo: check if open
    fclose(in);
}

void printPrompt()
{
    if (interactive)
        printf("alsh> ");
}

void parseCleanUp(char *freeme)
{

    if (freeme != NULL)
        free(freeme);
    return;
}

char *trim(char *s)
{
    while (isspace(*s))
        s++;

    if (*s == '\0')
        return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end))
        end--;

    end[1] = '\0';
    return s;
}

int walkPathAndLocateExecutable(char path[], char *executable)
{
    size_t j = 0;
    while (paths[j] != NULL)
    {
        snprintf(path, BUFF_SIZE, "%s/%s", paths[j], executable);
        if (access(path, X_OK) == 0)
        {
            return 1;
        }
        j++;
    }
    return -1;
}

void redirectOutputAndError(FILE *out)
{
    int outFileFd;
    if ((outFileFd = fileno(out)) == -1)
    {
        printError();
        return;
    }

    if (outFileFd != STDOUT_FILENO)
    {
        // redirect output
        if (dup2(outFileFd, STDOUT_FILENO) == -1)
        {
            printError();
            return;
        }
        // redirect error too..(as per proj requirements)
        if (dup2(outFileFd, STDERR_FILENO) == -1)
        {
            printError();
            return;
        }
        fclose(out);
    }
}

int parseLine()
{
    int i = 0, cnt = 0;
    FILE *out = stdout;
    while (line[i] != '\0')
    {
        if (line[i] == '>')
            cnt++;
        i++;
    }
    if (i == 0)
    {
        // nothing to do;
        //  printError();
        return 1;
    }
    if (cnt > 1)
    {
        printError();
        return 1;
    }

    char *cmd = strdup(line);
    char *freeme = cmd;

    cmd = trim(cmd);
    if (*cmd == '\0')
    {
        free(freeme);
        return 1;
    }
    char *redirectOut = NULL;

    if (cnt != 0) // means we have exactly one '<'
    {
        redirectOut = strdup(cmd);
        free(freeme);
        freeme = cmd = strsep(&redirectOut, ">");

        if (redirectOut == NULL || strlen(redirectOut) == 0 || *redirectOut == '\0')
        {
            free(freeme);
            printError();
            return 1;
        }
        if (strlen(cmd) == 0 || *cmd == '\0')
        {
            free(freeme);
            printError();
            return 1;
        }
        redirectOut = trim(redirectOut);

        if (strchr(redirectOut, ' ') != NULL)
        {
            // multiple output files
            printError();
            parseCleanUp(freeme);
            return 1;
        }

        out = fopen(redirectOut, "w");

        if (out == NULL)
        {
            printError();
            parseCleanUp(freeme);
            return 1;
        }

        cmd = trim(cmd);
    }

    char **ap, *argv[BUFF_SIZE];
    i = 0;
    cnt = 0;
    for (ap = argv; (*ap = strsep(&cmd, " \t")) != NULL;)
    {
        if (**ap != '\0')
        {
            i++;
            if (++ap >= &argv[BUFF_SIZE])
                break;
        }
    }

    if (argv[0] == NULL)
    {
        parseCleanUp(freeme);
        printError();
        return 1;
    }

    if (strcmp(argv[0], "cd") == 0)
    {
        if (i != 2)
        {
            printError();
        }
        else
        {
            if (chdir(argv[1]) == -1)
            {
                printError();
            }
        }
    }
    else if (strcmp(argv[0], "exit") == 0)
    {
        if (i != 1)
        {
            printError();
        }
        else
        {
            parseCleanUp(freeme);
            exit(0);
        }
    }
    else if (strcmp(argv[0], "path") == 0)
    {
        size_t j = 0;
        paths[0] = NULL;
        for (; j < i - 1; j++)
            paths[j] = strdup(argv[j + 1]);

        paths[j + 1] = NULL;
    }
    else
    {
        size_t j = 0;
        int found = 0;
        char path[BUFF_SIZE];

        if (walkPathAndLocateExecutable(path, argv[0]) == -1)
        {
            printError();
        }
        else
        {
            pid_t rc = fork();

            if (rc < 0)
            {
                // fork failed
                printError();
            }
            else if (rc == 0)
            {
                // child
                redirectOutputAndError(out);

                if (execv(path, argv) == -1)
                {
                    printError();
                }
            }
            else
            {
                // parent
                waitpid(rc, NULL, 0);
            }
        }
    }

    parseCleanUp(freeme);
    return 1;
}

int main(int argc, char *argv[])
{

    if (argc == 1)
    {
        in = stdin;
        interactive = 1;
        printPrompt();
    }
    else if (argc == 2)
    {
        in = fopen(argv[1], "r");
        if (in == NULL)
        {
            printError();
            exit(1);
        }
    }
    else
    {
        printError();
        exit(1);
    }

    size_t linecap = 0;
    ssize_t linelen;
    atexit(clean);
    while ((linelen = getline(&line, &linecap, in)) > 0)
    {

        if (line[linelen - 1] == '\n')
        {
            line[linelen - 1] = '\0';
        }

        int ok = parseLine();
        printPrompt();
    }

    return 0;
}