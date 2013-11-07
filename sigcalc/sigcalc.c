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

static void wakeUpReader(int signo) {
    pthread_kill(r, SIGUSR1);
}


static void wakeUpCalculator(int signo) {
    pthread_kill(c, SIGUSR2);
}

static void interruptCalculator(int signo) {
    pthread_kill(c, SIGALRM);
}

static void interruptReader(int signo) {
    pthread_kill(r, SIGINT);
}

static void handler(int signo) {

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
        sleep(1);

        if (sig == SIGUSR1) {
            fscanf(stream, "%d %d", &sharedData->a, &sharedData->b);

            if(feof(stream)) {
                fclose(stream);
                kill(0, SIGALRM);
                sigwait(&set, &sig);
                if(sig == SIGINT) {
                    printf("Goodbye from Reader.\n");
                    break;
                }
            }

            printf("Thread 1 submitting : %d %d\n", sharedData->a,
                   sharedData->b);
            kill(0, SIGUSR2);
        }

        // Wait for main to tell to run.
        sigwait(&set, &sig);
    }
    return ((void *)NULL);
}

// Handler for the calculator thread.
static void * calculator(void *sharedData_in) {
    int sig;
    sigset_t set;
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGUSR2);
    sigprocmask(SIG_BLOCK, &set, NULL);

    // Cast the shared data back to type sharedData.
    sharedData_t *sharedData = (sharedData_t *)sharedData_in;

    while(1) {
        sleep(1);

        if(sig == SIGUSR2) {
            int calculation = sharedData->a + sharedData->b;
            printf("Thread 2 calculated : %d\n", calculation);
            kill(0, SIGUSR1);
        } else if(sig == SIGALRM) {
            printf("Goodbye from Calculator.\n");
            kill(0, SIGPIPE);
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
    sigdelset(&set,SIGTSTP);
    sigprocmask(SIG_BLOCK, &set, NULL);
    sigset(SIGUSR1, wakeUpReader);
    sigset(SIGUSR2, wakeUpCalculator);
    sigset(SIGALRM, interruptCalculator);
    sigset(SIGPIPE, interruptReader);

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
