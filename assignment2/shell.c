#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 80

int main()
{
    char *args[MAX_LINE / 2 + 1];   
    int should_run = 1;              

    char *history[5];                
    int history_count = 0;          

    for (int i = 0; i < 5; i++) 
    {
        history[i] = malloc(MAX_LINE + 1);
        
        if (history[i] == NULL) {
            return 1;
        }
        history[i][0] = '\0';
    }

    while (should_run) {

        char buf[MAX_LINE];

        printf("osh> ");
        fflush(stdout);

        // get information from input buffer and put into history. print if requested
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        // Exit condition
        if (strcmp(buf, "exit") == 0) {
            should_run = 0;
            break;
        }

        // print command history 
	    if (strcmp(buf, "history") == 0) {
    		int count = (history_count < 5) ? history_count : 5;
    		for (int i = 0; i < count; i++) {
        		int index = (history_count - 1 - i) % 5;
        		if (index < 0) index += 5; // handle wrap-around safely
        		printf("%d %s\n", history_count - i, history[index]);
    		}
    		continue;
	    }

        // handle !!
        if (strcmp(buf, "!!") == 0) {
            if (history_count == 0) {
                printf("No commands in history.\n");
                continue;
            } else {
                // replace !! with most recent command
                snprintf(buf, MAX_LINE, "%s", history[(history_count - 1) % 5]);
                printf("%s\n", buf);
            }
        }

        // add current command to history
        snprintf(history[history_count % 5], MAX_LINE, "%s", buf);
        history_count++;

        // Detect background execution (&)
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

    // free allocated memory before exit
    for (int i = 0; i < 5; i++) {
        free(history[i]);
    }

    return 0;
}