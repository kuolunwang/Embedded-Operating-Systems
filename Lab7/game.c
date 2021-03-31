#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct
{
	int guess;
	char result[8];
} data;

data *guess_number;
int guess, stop = 1;

void handler(int signo, siginfo_t *info, void *context)
{
	/*show the process ID sent signal*/
	// Process(ID) sent SIGUSR1.

	if (guess_number->guess == guess)
	{
		stop = 0;
	}
	else if (guess_number->guess > guess)
	{
		strcpy(guess_number->result, "smaller");
		printf("[game] Guess %d %s \n", guess_number->guess, guess_number->result);
	}
	else if (guess_number->guess < guess && guess_number->guess > 0)
	{
		strcpy(guess_number->result, "bigger");
		printf("[game] Guess %d, %s \n", guess_number->guess, guess_number->result);
	}
}

void shm_create(int input_key)
{
	char c;
	int shmid;
	key_t key;
	data *sh_guess_number;
	int retval;

	/* We ’ll name our shared memory segment "5678" */
	key = input_key;

	/* Create the segment */
	if ((shmid = shmget(key, 1, IPC_CREAT | 0666)) < 0)
	{
		perror("shmget");
		exit(1);
	}

	/* Now we attach the segment to our data space */
	if ((sh_guess_number = shmat(shmid, NULL, 0)) == (data *)-1)
	{
		perror("shmat");
		exit(1);
	}
	// Server create and attach the share memory.

	/* Now put some things into the memory for the other process to read */
	guess_number = sh_guess_number;
	guess_number->guess = -1;
	strcpy(guess_number->result, "init");

	// Server write guess_number to share memory.

	/*
	* Finally , we wait until the other process changes the first
	* character of our memory to ’*’, indicating that it has read
	* what we put there .
	*/
	// Waiting other process read the share memory...

	while (stop)
	{
		sleep(1);
	}

	strcpy(guess_number->result, "bingo");
	printf("[game] Guess %d, %s \n", guess_number->guess, guess_number->result);

	// Server read data from the share memory.
	/* Detach the share memory segment */
	shmdt(sh_guess_number);
	/* Destroy the share memory segment */
	// Server destroy the share memory.
	retval = shmctl(shmid, IPC_RMID, NULL);
	if (retval < 0)
	{
		fprintf(stderr, "Server remove share memory failed \n");
		exit(1);
	}
}

int main(int argc, char **argv)
{
	int key_number;
	struct sigaction my_action;

	key_number = atoi(argv[1]);
	guess = atoi(argv[2]);

	/*register hanler to SIGUSR1*/
	memset(&my_action, 0, sizeof(struct sigaction));
	my_action.sa_flags = SA_SIGINFO;
	my_action.sa_sigaction = handler;

	sigaction(SIGUSR1, &my_action, NULL);

	printf("[game] Guess PID: %d \n", getpid());

	shm_create(key_number);
	return 0;
}
