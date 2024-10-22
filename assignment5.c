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

// Make sure to add random function
/*int randomGaussian(int mean, int stddev) {
	double mu = 0.5 + (double) mean;
	double sigma = fabs((double) stddev);
	double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
	double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
	if (rand() & (1 << 5)) 
		return (int) floor(mu + sigma * cos(f2) * f1);
	else            
		return (int) floor(mu + sigma * sin(f2) * f1);
}*/

int main(int argc, char *argv[]){	
	// Relevant Variables
	sem_t *chopsticks; // Shared memory for chopsticks
	sem_t *mutex; // Make sure only one process is in the critical section
	pid_t pids[PHILOSOPHERS]; // Array of pids representing each philosopher
	int runningPhilosophers = PHILOSOPHERS; // Tracks active running processes. 
	int *secondsElapsed; // Second of eating elapsed 

	// Delcare new shared memory for Philosophers, eating time and attach chopsticks and seoncdsElapsed to shared memory. 
	int shmid = shmget(IPC_PRIVATE, sizeof(sem_t) * PHILOSOPHERS + sizeof(sem_t), IPC_CREAT | 0666); 
	int shmidEat = shmget(IPC_PRIVATE, sizeof(int) * PHILOSOPHERS, IPC_CREAT | 0666); 
	chopsticks = (sem_t*)shmat(shmid, 0, 0); // Attach chopsticks to process
	mutex = (sem_t*)(chopsticks + PHILOSOPHERS); // Place mutex after chopstics array in shared memory
	secondsElapsed = (int*)shmat(shmidEat, 0, 0);  // Attach secondsElpased to process

	// Attached secondElpased to shared memory so parent can gain access to philosopher eating time 
	for (int i = 0; i < PHILOSOPHERS; i++) {
		secondsElapsed[i] = 0; 
	}
	
	// Error check for shmat (chopsticks)
	if (chopsticks == (void*) - 1) {
		write(2, strerror(errno), strlen(strerror(errno))); 
		return -errno; 
	}

	// Error check for shmat (secondsElapsed)
	if (secondsElapsed == (void*) - 1) {
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
			while (secondsElapsed[i] < 100) {
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
				printf("Philosopher %d (PID: %d) is currently eating \n", i + 1, getpid()); 
				sleep(abs(eatingTime)); // Phislosopher eats
				secondsElapsed[i] += eatingTime; 

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
					printf("Philosopher %d (PID: %d) is done and has left the table. Total Eating Time: %d seconds\n", i + 1, getpid(), secondsElapsed[i]); 
					pids[i] = 0; 
					runningPhilosophers--; 
				}
			}
		}
		sleep(1); 
	}
	return 0;
}

