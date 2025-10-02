#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_LINE 80

int main()
{
    char *args[MAX_LINE / 2 + 1];

    bool running = true;

    char *prev[5];
    int prev_counter = 0;

    for (int i = 0; i < 5; i++)
    {
        prev[i] = malloc(sizeof(char) * 80 + 1);

        if (prev[i] == NULL)
        {
            return 1;
        }
    }

    while (running)
    {
        char buf[MAX_LINE];

        printf("osh> ");
        fflush(stdout);

        // get information from input buffer and put into history. print if requested
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';
        if (strcmp(buf, "history") == 0)
        {
            for (int i = prev_counter; i < prev_counter + 5; i++)
            {
                printf("%s\n", prev[i % 5]);
            }
            continue;
        }

        snprintf(prev[prev_counter % 5], 81, "%s", buf);
        prev_counter++;

        // copy previous cmd into
        // printf("%d\n", strcmp(buf, "!!"));
        if (strcmp(buf, "!!") == 0)
        {
            snprintf(buf, MAX_LINE, "%s", prev[(prev_counter - 1) % 5]);
        }

        // see if the function is concurrent, trim off end
        bool concurrent = false;
        int i = strlen(buf) - 1;

        while (buf[i] == ' ')
        {
            i -= 1;
        }
        if (i > 0)
        {

            if (buf[i] == '&' && buf[i - 1] == ' ')
            {
                concurrent = buf[i] == '&';
                buf[i - 1] = '\0';
            }
        }

        char *args[128];
        args[0] = strtok(buf, " ");
        int currarg = 1;
        while (args[currarg - 1] != NULL)
        {
            args[currarg] = strtok(NULL, " ");
            currarg++;
        }

        // execute command, wait if necessary

        fflush(stdout);
        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(args[0], args);
            exit(0);
        }

        if (pid > 0 && !concurrent)
        {
            waitpid(pid, NULL, 0);
        }
    }
}
