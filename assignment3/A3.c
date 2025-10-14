#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>

// semaphore for getting seats
// semaphore for waiting for TA
// semaphore for TA helping

sem_t seats;
sem_t enter_ta_room;
sem_t ta_helping;

int view_student;

void *student(void *param)
{
    int student_id = *((int *)param);
    while (true)
    {
        // the student starts by programming for a random amount of time
        unsigned int programming_time = (unsigned int)(5 * (rand() / (double)RAND_MAX) + 5);

        if (3 == student_id)
        {
            printf("%d: Programming for %u seconds\n", student_id, programming_time);
        }
        sleep(programming_time); // programming
        if (3 == student_id)
        {
            printf("%d: Done Programming\n", student_id);
        }

        // try to get seat. otherwise continue
        if (3 == student_id)
        {
            printf("%d: Trying to get seat\n", student_id);
        }
        if (sem_trywait(&seats) != 0)
        {
            if (student_id == 3)
            {
                printf("%d: Couldn't get seat\n", student_id);
            }
            continue;
        }
        if (3 == student_id)
        {

            printf("%d: Got seat\n", student_id);
        }

        if (student_id == 3)
        {

            printf("%d: Waitin for ta\n", student_id);
        }
        sem_wait(&enter_ta_room);
        if (student_id == 3)
        {
            printf("%d: Admitted by ta\n", student_id);
        }
        sem_post(&seats);
        // wait for the TA on another semaphore
        // once the thread gets selected, increase the seats semaphore

        if (student_id == 3)
        {
            printf("%d: Being helped by ta\n", student_id);
        }
        sem_wait(&ta_helping);
        if (student_id == 3)
        {
            printf("%d: Done being helped by ta\n", student_id);
        }

        // wait on the TA help semaphore until TA lifts it
    }
}

void *ta()
{
    while (true)
    {
        int no_student;
        if (sem_getvalue(&enter_ta_room, &no_student) != 0)
        {
            continue;
        }

        if (no_student == 1)
        {
            continue;
        }

        double helping_time = 5 * rand() / (double)RAND_MAX + 5;

        printf("Helping student\n");
        sleep(helping_time);
        printf("Done helping student\n");

        sem_post(&ta_helping);

        sem_post(&enter_ta_room);
    }
}

int main(int argv, char **argc)
{
    srand(time(NULL));

    if (argv != 3)
    {
        return 1;
    }

    if (sem_init(&seats, atoi(argc[2]), atoi(argc[2])) != 0)
    {
        printf("Semaphore initialization error.\n");
        return 1;
    }
    if (sem_init(&enter_ta_room, 1, 1) != 0)
    {
        printf("Semaphore initialization error.\n");
        return 1;
    }
    if (sem_init(&ta_helping, 0, 0) != 0)
    {
        printf("Semaphore initialization error.\n");
        return 1;
    }

    pthread_t student_threads[atoi(argc[1])];

    for (int i = 0; i < atoi(argc[1]); i++)
    {
        int *student_id = malloc(sizeof(int));
        *student_id = i;
        pthread_create(&student_threads[i], NULL, student, student_id);
    }

    pthread_t ta_thread;
    pthread_create(&ta_thread, NULL, ta, NULL);

    for (int i = 0; i < atoi(argc[1]); i++)
    {
        pthread_join(student_threads[i], NULL);
    }
    pthread_join(ta_thread, NULL);
}
