#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

sem_t seats;            // Available chairs in hallway
sem_t students_waiting; // Students signal TA that they're waiting
sem_t ta_ready;         // TA signals that they're ready for next student
sem_t help_complete;    // TA signals that help session is complete

int num_seats;

/*
 Students alternate between programming and seeking help from TA
*/
void *student(void *param) {
    int student_id = *((int *)param);
    free(param);
    
    while (true) {
        // the student starts by programming for a random amount of time
        unsigned int programming_time = (unsigned int)(5 * (rand() / (double)RAND_MAX) + 5);
        printf("Student %d: Programming for %u seconds\n", student_id, programming_time);
        sleep(programming_time);
        
        printf("Student %d: Done programming, trying to get a seat...\n", student_id);
        
        // // try to get seat. otherwise to back to the start of the loop
        if (sem_trywait(&seats) != 0) {
            printf("Student %d: No seat available, will try later.\n", student_id);
            continue;
        }
        
        printf("Student %d: Got a seat, waiting for TA.\n", student_id);
        
        // signal TA that a student is waiting (wakes TA if sleeping)
        sem_post(&students_waiting);
        
        // wait until TA is ready to help this student
        sem_wait(&ta_ready);
        
        // once the student enters room, their seat is freed
        sem_post(&seats);
        
        printf("Student %d: Being helped by TA.\n", student_id);
        
        // wait for ta to signal that they are done helping the student
        sem_wait(&help_complete);
        
        printf("Student %d: Done being helped, back to programming.\n", student_id);
    }
    
    return NULL;
}

/*
 TA sleeps when no students waiting, helps students when they arrive
*/
void *ta(void *param) {
    (void)param;
    
    while (true) {
        // TA waits for a student (sleeps if no students)
        printf("TA: No students waiting, taking a nap...\n");
        sem_wait(&students_waiting);
        
        printf("TA: Student arrived, ready to help.\n");
        
        // signal student that TA is ready
        sem_post(&ta_ready);
        
        // help the student for random time 
        unsigned int help_time = (unsigned int)(5 * (rand() / (double)RAND_MAX) + 1);
        printf("TA: Helping student for %u seconds.\n", help_time);
        sleep(help_time);
        
        printf("TA: Done helping student.\n");
        
        // signal student that help is complete
        sem_post(&help_complete);
    }
    
    return NULL;
}

int main(int argc, char **argv) {
    srand(time(NULL));
    
    if (argc != 3) {
        printf("Usage: %s <num_students> <num_chairs>\n", argv[0]);
        return 1;
    }
    
    int num_students = atoi(argv[1]);
    num_seats = atoi(argv[2]);
    
    if (num_students <= 0 || num_seats <= 0) {
        fprintf(stderr, "Error: Number of students and chairs must be positive.\n");
        return 1;
    }
    

    printf("Students: %d, Chairs: %d\n\n", num_students, num_seats);
    
    // initialize semaphores
    if (sem_init(&seats, 0, num_seats) != 0) {
        fprintf(stderr, "Error: Failed to initialize seats semaphore.\n");
        return 1;
    }
    
    if (sem_init(&students_waiting, 0, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize students_waiting semaphore.\n");
        return 1;
    }
    
    if (sem_init(&ta_ready, 0, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize ta_ready semaphore.\n");
        return 1;
    }
    
    if (sem_init(&help_complete, 0, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize help_complete semaphore.\n");
        return 1;
    }
    
    // create student and ta threads
    pthread_t ta_thread;
    if (pthread_create(&ta_thread, NULL, ta, NULL) != 0) {
        fprintf(stderr, "Error: Failed to create TA thread.\n");
        return 1;
    }
    
    pthread_t student_threads[num_students];
    for (int i = 0; i < num_students; i++) {
        int *student_id = malloc(sizeof(int));
        if (student_id == NULL) {
            fprintf(stderr, "Error: Memory allocation failed.\n");
            return 1;
        }
        *student_id = i + 1;
        
        if (pthread_create(&student_threads[i], NULL, student, student_id) != 0) {
            fprintf(stderr, "Error: Failed to create student thread %d.\n", i + 1);
            free(student_id);
            return 1;
        }
        
    }
    
    // join all threads
    pthread_join(ta_thread, NULL);
    for (int i = 0; i < num_students; i++) {
        pthread_join(student_threads[i], NULL);
    }
    
    // cleanup semaphores
    sem_destroy(&seats);
    sem_destroy(&students_waiting);
    sem_destroy(&ta_ready);
    sem_destroy(&help_complete);
    
    return 0;
}