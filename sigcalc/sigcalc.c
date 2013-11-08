//
// CA321 - OS Design and Implementation
// Assignment 1 - POSIX threads and signal handling
//
// @author Ian Duffy, 11356066
// @author Darren Brogan, 11424362
// @author Peter Morgan, 11314976
//
// This project is our own work. We have not recieved assistance beyond what is
// normal, and we have cited any sources from which we have borrowed. We have
// not given a copy of our work, or a part of our work, to anyone. We are aware
// that copying or giving a copy may have serious consequences.
//

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sysexits.h>

#define SLEEP 10000

pthread_t r, c;

// Data structure for passing parameters to the threads.
typedef struct {
    int a, b;
    char* filename;
} sharedData_t;

// Signal handler to wake up reader from main.
static void wakeUpReader(int signo) {
    pthread_kill(r, SIGUSR1);
}

// Signal Handler to wake up Calculator from main.
static void wakeUpCalculator(int signo) {
    pthread_kill(c, SIGUSR2);
}

// Signal Handler to end both Reader and Calculator.
static void interruptThreads(int signo) {
    pthread_kill(c, SIGINT);
    pthread_kill(r, SIGINT);
}


// Handler for the reader thread.
static void * reader(void *sharedData_in) {

    int sig = SIGUSR1;
    sigset_t set;
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR1);
    sigprocmask(SIG_BLOCK, &set, NULL);

    // Cast the shared data back to type sharedData.
    sharedData_t *sharedData = (sharedData_t *)sharedData_in;

    // Open the file for reading.
    FILE *stream;
    stream = fopen(sharedData->filename, "r");
    if(stream == NULL) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    while(1) {
        usleep(rand() % SLEEP);

        if (sig == SIGUSR1) {
            // Read the two numbers.
            fscanf(stream, "%d %d", &sharedData->a, &sharedData->b);

            // Detect the end of the file.
            if(feof(stream)) {
                // Close the file.
                fclose(stream);
                // Tell main to interrupt all threads.
                kill(0, SIGALRM);
                sigwait(&set, &sig);
                if(sig == SIGINT) {
                    printf("Goodbye from Reader.\n");
                    break;
                }
            }

            // Print message to user showing read.
            printf("Thread 1 submitting : %d %d\n", sharedData->a, sharedData->b);

            // Inform Calculator through Main that Data is available.
            kill(0, SIGUSR2);
        }

        // Wait until more data is required.
        sigwait(&set, &sig);
    }
    return ((void *)NULL);
}

// Handler for the calculator thread.
static void * calculator(void *sharedData_in) {
    int sig;
    sigset_t set;
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    // Cast the shared data back to type sharedData.
    sharedData_t *sharedData = (sharedData_t *)sharedData_in;

    while(1) {
        usleep(rand() % SLEEP);

        if(sig == SIGUSR2) {
            // Do the calculation.
            int calculation = sharedData->a + sharedData->b;

            // Inform the user of the outcome.
            printf("Thread 2 calculated : %d\n", calculation);

            // Inform Reader from Main that more data is needed.
            kill(0, SIGUSR1);
        } else if(sig == SIGINT) {
            // Terminate.
            printf("Goodbye from Calculator.\n");
            break;
        }

        // Wait for main to tell to run.
        sigwait(&set, &sig);
    }
    return ((void *)NULL);
}

int main(int argc, char *argv[]) {
    int sig;
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, NULL);
    sigset(SIGUSR1, wakeUpReader);
    sigset(SIGUSR2, wakeUpCalculator);
    sigset(SIGALRM, interruptThreads);

    // Check that a valid command line argument was passed.
    if(argc != 2) {
        // Display an error to stderr, detailing valid usage.
        fprintf(stderr, "Usage: ./sigcalc file\n");

        // Exit returning the sysexits value for invalid command usage.
        exit(EX_USAGE);
    }

    sharedData_t sharedData;
    sharedData.filename = argv[1];

    pthread_create(&r,NULL,reader,(void *) &sharedData);
    pthread_create(&c,NULL,calculator,(void *) &sharedData);

    pthread_join(r,NULL);
    pthread_join(c,NULL);

    return 0;
}
