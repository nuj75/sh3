

#define MAX_LINE 80

int main()
{
    char *args[MAX_LINE / 2 + 1];

    int running = 1;

    while (running)
    {
        printf("osh> ");
        scanf("%s", args);

        printf("%s", args);
    }
}