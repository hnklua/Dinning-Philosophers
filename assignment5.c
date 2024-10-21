#include "random.h"
#include <stdio.h>
#include <semaphore.h> 
#include <unistd.h> 
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <sys/sem.h> 
#include <sys/shm.h> 
#include <sys/wait.h>

#define PHILOSOPHERS 5

int main(int argc, char *argv[]){	
	// Relevant Variables
	sem_t *chopsticks; // Array of Semaphores for chopsticks
	sem_t *mutex; // Make sure only one process is in the critical section
	pid_t pids[PHILOSOPHERS]; // Array of pids representing each philosopher
	int runningPhilosophers = PHILOSOPHERS; // Tracks active running processes. 
	int secondsElapsed = 0; // Second of eating elapsed 

	// Create a set of semaphores based on the constant for Philosophers. 
	//int shmid = semget(IPC_PRIVATE, sizeof(sem_t) * PHILOSOPHERS, S_IRUSR | S_IWUSR); 
	int shmid = shmget(IPC_PRIVATE, sizeof(sem_t) * PHILOSOPHERS + sizeof(sem_t), IPC_CREAT | 0666); 
	chopsticks = (sem_t*)shmat(shmid, 0, 0); // Attach chopsticks to process
	mutex = (sem_t*)(chopsticks + PHILOSOPHERS); // Place mutex after chopstics array in shared memory
	
	// Error check for shmat
	if (chopsticks == (void*) - 1) {
		write(2, strerror(errno), strlen(strerror(errno))); 
		return -errno; 
	}

	// Initalize Chopstick semaphores
	for (int k = 0; k < PHILOSOPHERS; k++) {
		if (sem_init(&chopsticks[k], 1 , 1) == -1) { // Initalizing the sempahores. Indicating that 2 can be held at most
			write(2, strerror(errno), strlen(strerror(errno))); 
			return -errno; 
		} 
	}

	// Initialize mutex semaphore
	if (sem_init(mutex, 1, 1) == -1) {
		write(2, strerror(errno), strlen(strerror(errno))); 
		return -errno; 
	}

	// Fork appropriate amount of philosophers
	for (int i = 0; i < PHILOSOPHERS; i++) {
		pid_t pid = fork(); 
		// Fork number of necessary processes
		if (pid == -1) {
			write(2, strerror(errno), strlen(strerror(errno))); 
			return -errno; 
		}
		// Philosopher process 
		if (pid == 0) {
			srand(time(NULL) ^ getpid()); ; // Random number generator
			while (secondsElapsed < 100) {
				int eatingTime = randomGaussian(9, 3); 
				int thinkingTime = randomGaussian(11, 7); 
				
				// Thinking state
				sleep(abs(thinkingTime)); // Thinking
				printf("Philosopher %d (PID: %d) is currently thinking. Thinking Time: %d seconds\n", i + 1, getpid(), abs(thinkingTime)); 

				// Attempting to eat State
				sem_wait(mutex); 
				sem_wait(&chopsticks[(i + 1) % PHILOSOPHERS]); // Pick up left one 
				sem_wait(&chopsticks[i]); // Pick up right chopstick 
				printf("Philosopher %d (PID: %d) is attempting to pick up chopsticks \n", i + 1, getpid()); 
				sem_post(mutex); // Reopen critical section

				// Eating state
				sleep(abs(eatingTime)); // Phislosopher eats
				printf("Philosopher %d (PID: %d) is currently eating \n", i + 1, getpid()); 
				secondsElapsed += eatingTime; 

				// Put down chopsticks
				sem_post(&chopsticks[(i + 1) % PHILOSOPHERS]); // Release left chopstick 
				sem_post(&chopsticks[i]); // Release right chopsticks
				printf("Philosopher %d (PID: %d) put down their chopsticks. Eating Time: %d seconds\n", i + 1, getpid(), abs(eatingTime)); 
			}
			exit(0); // Philosopher process ends
		}
		// Store philosopher PID in parent process
		pids[i]= pid; 
	} 
	// Keep tabs on running philosophers 
	while (runningPhilosophers != 0) {
		for (int i = 0; i < PHILOSOPHERS; i++) {
			if (pids[i] != 0) {
				int status; // Status of process
				pid_t result = waitpid(pids[i], &status, WNOHANG); 

				if (result == pids[i]) {
					printf("Philosopher %d (PID: %d) is done and has left the table\n", i + 1, getpid()); 
					pids[i] = 0; 
					runningPhilosophers--; 
				}
			}
		}
		sleep(1); 
	}
	return 0;
}

