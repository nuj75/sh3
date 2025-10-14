#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>

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
        printf("%d: Programming for %u seconds\n", student_id, programming_time);
        sleep(programming_time); // programming
        printf("%d: Done Programming\n", student_id);

        // try to get seat. otherwise to back to the start of the loop
        printf("%d: Trying to get seat\n", student_id);
        if (sem_trywait(&seats) != 0)
        {
            printf("%d: Couldn't get seat\n", student_id);
            continue;
        }
        printf("%d: Got seat\n", student_id);

        // wait for the ta to admit student into room
        printf("%d: Waiting for ta\n", student_id);
        sem_wait(&enter_ta_room);
        printf("%d: Admitted by ta\n", student_id);
        // once the student enters room, their seat is freed
        sem_post(&seats);

        // wait for ta to signal that they are done helping the student
        printf("%d: Being helped by ta\n", student_id);
        sem_wait(&ta_helping);
        printf("%d: Done being helped by ta\n", student_id);
    }
}

void *ta()
{
    while (true)
    {
        // if the semaphore is at one, no students have entered the room:
        // go back to the start of the loop
        int no_student;
        if (sem_getvalue(&enter_ta_room, &no_student) != 0)
        {
            continue;
        }
        if (no_student == 1)
        {
            continue;
        }

        // help student for a set amount of time
        double helping_time = 5 * rand() / (double)RAND_MAX + 5;
        printf("Helping student\n");
        sleep(helping_time);
        printf("Done helping student\n");

        // signal to the student that the ta is done helping. this awakes
        // the waiting student thread
        sem_post(&ta_helping);

        // signal to student threads that are outside of the room
        sem_post(&enter_ta_room);
    }
}

int main(int argv, char **argc)
{
    // set seed
    srand(time(NULL));

    // take in arguments for number of students and seats
    if (argv != 3)
    {
        return 1;
    }

    // initalize semaphores
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

    // create student and ta threads
    pthread_t student_threads[atoi(argc[1])];
    for (int i = 0; i < atoi(argc[1]); i++)
    {
        int *student_id = malloc(sizeof(int));
        *student_id = i;
        pthread_create(&student_threads[i], NULL, student, student_id);
    }
    pthread_t ta_thread;
    pthread_create(&ta_thread, NULL, ta, NULL);

    // join all threads
    for (int i = 0; i < atoi(argc[1]); i++)
    {
        pthread_join(student_threads[i], NULL);
    }
    pthread_join(ta_thread, NULL);
}
