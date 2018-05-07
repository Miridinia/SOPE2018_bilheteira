#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h> 
#include <sys/stat.h>
#include <pthread.h> 
#include <mqueue.h>
#include <semaphore.h>
#include <errno.h>

#include "ticket.h"
// $ server <num_room_seats> <num_ticket_offices> <open_time>

struct server_args_t {
	int num_room_seats;
	int num_ticket_offices;
	int open_time;
};

struct client_args_t {
	int pid;
	int num_wanted_seats;
	int pref_seat_list [MAX_CLI_SEATS];
	int num_pref_seats;
};

typedef struct  {
	int num_room_seats;
	int *seats_taken;
} Seat;

void read_msg(char *msg, struct client_args_t * args);
void parse_args(char *argv[], struct server_args_t * args);
void print_args(struct server_args_t * args);
void createFIFO(const char *pathname);
int openFIFO(const char *pathname, mode_t mode);
void writeOnFIFO(int fd, char *message, int messagelen);
int readOnFIFO(int fd, char *str);
void closeFIFO(int fd);
void killFIFO(char *pathname);
void *bilheteira(void *threadId);
void checkResult(char *string, int err);

int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);

//Globals
Seat seats;
int conditionMet = 0;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char *message;

static void ALARMhandler(int sig)
{
	int res;
	printf("Ticket Office Closed!\n");
	killFIFO(FIFO_NAME_CONNECTION);
	res = pthread_cond_destroy(&cond);
	checkResult("pthread_cond_destroy()\n", res);
	res = pthread_mutex_destroy(&mutex);
	checkResult("pthread_mutex_destroy()\n", res);
	exit(0);
}



int main(int argc, char *argv[]) {
	printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

	int res; //????

	if (argc == 4) {
		printf("ARGS: %s | %s | %s\n", argv[1], argv[2], argv[3]);
	} else {
		return -1;
	}

	struct server_args_t args;

	parse_args(argv, &args);

	print_args(&args);

	//define Seat
	seats.num_room_seats = args.num_room_seats;

	//Creats fifo requests
	createFIFO(FIFO_NAME_CONNECTION);

	//creating Bilheteiras
	pthread_t tid[args.num_ticket_offices]; 
	int rc, t; 
	int thrArg[args.num_ticket_offices]; 
	for(t=1; t<= args.num_ticket_offices; t++){ 
		thrArg[t-1] = t; 
		rc = pthread_create(&tid[t-1], NULL, bilheteira, &thrArg[t-1]);
		if (rc) { 
			printf("ERROR; return code from pthread_create() is %d\n", rc); 
			exit(1); 
		} 
	} 

 	//creating alarm for the open time of the bilheiteiras
	signal(SIGALRM, ALARMhandler);
	alarm(args.open_time);

	//creating FIFO
	int fd = openFIFO(FIFO_NAME_CONNECTION, O_RDONLY);
	sleep(1); //to wait threads for doing something??
	
	int check = 0;
	res = pthread_mutex_lock(&mutex); //making everything sleep??
	checkResult("pthread_mutex_lock()\n", res);
	while(1) {

		char str[100];
		while(readOnFIFO(fd, str) && check == 0) {
			check = 1;
			printf("%s \n", str);
		} 
		if(check) {
			message = str;
			conditionMet = 1;
			res = pthread_cond_broadcast(&cond); //send everyone a message
			checkResult("pthread_cond_broadcast()\n", res);
			res = pthread_mutex_unlock(&mutex);
			checkResult("pthread_mutex_unlock()\n", res);
			printf("Main thread: waiting for threads and cleanup\n");
			
			while(conditionMet != 0){
				//printf("esperando...\n");
			}
			check = 0;
		}

	}
	

	//teste
	/*printf("PID: %d\n", reservation.pid);
	printf("Number of Wanted Seats: %d\n", reservation.num_wanted_seats);
	printf("Preferred Seats:");
	for(size_t i = 0; i < reservation.num_pref_seats; i++) {
		printf(" %d", reservation.pref_seat_list[i]);
	}
	printf("\n");*/

	closeFIFO(fd);
	killFIFO(FIFO_NAME_CONNECTION);

	  //return 0;
}

void read_msg(char *msg, struct client_args_t * args) {
	args->num_pref_seats = 0;
	char* next_seat = strtok(msg, " ");
	sscanf(next_seat, "%d", &(args->pid));
	next_seat = strtok(NULL, " ");
	sscanf(next_seat, "%d", &(args->num_wanted_seats));
	next_seat = strtok(NULL, " ");
	while(next_seat != NULL) {
		sscanf(next_seat, "%d", &(args->pref_seat_list[args->num_pref_seats]));
		args->num_pref_seats++;
		next_seat = strtok(NULL, " ");
	}
}

void parse_args(char* argv[], struct server_args_t * args) {
	sscanf(argv[1], "%d", &(args->num_room_seats));
	sscanf(argv[2], "%d", &(args->num_ticket_offices));
	sscanf(argv[3], "%d", &(args->open_time));
}

void print_args(struct server_args_t * args) {
	printf("Number of Seats in Room: %d\n", args->num_room_seats);
	printf("Number of Ticket Offices: %d\n", args->num_ticket_offices);
	printf("Open Time: %d\n", args->open_time);
	printf("\n");
}

void *bilheteira(void *threadId) { 
	int res;
	int threadNum = *(int*)threadId;
 	printf("Ticket Office nº %2d: Created\n", threadNum); 
	
	//receiving the message
	
	while(1) {
		char msggg[MAX_TOKEN_LEN];
		printf("Thread %d blocked because condition is not met\n", threadNum);
		conditionMet = 0;
		res = pthread_cond_wait(&cond, &mutex);
		checkResult("pthread_cond_wait()\n", res);
		if(conditionMet == 1) {
			conditionMet = 2;
			strcpy(msggg, message);
				
			printf("Thread %d executing critical section for 1 seconds ...\n",threadNum);
			printf("\n\n*************RECEBEU**********************\n");
			printf("bilheteira: %i, recebeu %s\n", threadNum, msggg);
			printf("******************************************\n\n\n");
			sleep(1);			
		}	

	}
} 

void checkResult(char *string, int err) {
	if(err != 0) {
		printf("Error %d on %s\n", err, string);
		exit(EXIT_FAILURE);
	}
	return;
}

int char_to_int(char* to_convert) {
	int converted = atoi(to_convert);
	return converted;
}

void createFIFO(const char *pathname) {
	if(mkfifo(pathname , PERMISSIONS_FIFO) != 0){
		printf("ERROR: COULDN'T CREATE FIFO\n");
		exit(0);
	}	
}

int openFIFO(const char *pathname, mode_t mode) {
	int fd = open(pathname, mode); 
	if(fd == -1) {
		printf("ERROR: COULDNT OPEN FIFO\n");
	}
	return fd;
}

void writeOnFIFO(int fd, char *message, int messagelen) {
	if(write(fd,message,messagelen) != messagelen) {
		printf("ERROR: COULDN'T WRITE ON FIFO\n");
		exit(0);
	}
}

int readOnFIFO(int fd, char *str) {
	int n; 
	do {
		n = read(fd,str,1);     
	} while (n>0 && *str++ != '\0'); 

	return (n>0); 
}

void closeFIFO(int fd) {
	if(close(fd) < 0) {
		printf("ERROR: COULDN'T CLOSE FIFO\n");
		exit(0);
	}
}

void killFIFO(char *pathname) {
	if(unlink(pathname) < 0) {
		printf("ERROR: COULDN'T DESTROY FIFO\n");
		exit(0);
	}
}