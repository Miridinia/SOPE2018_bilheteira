#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/file.h> 
#include <sys/stat.h>
#include <pthread.h> 

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

int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);

static void ALARMhandler(int sig)
{
	printf("Ticket Office Closed!\n");
	killFIFO(FIFO_NAME_CONNECTION);
	exit(0);
}

//Globals
Seat seats;
char* messages[MAX_WAITING_LIST];
int msg_count = 0;

int main(int argc, char *argv[]) {
	printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

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

	while(1) {
		char str[100];
		while(readOnFIFO(fd, str)) 
		printf("%s \n", str);

		//defining the msg
		//messages[msg_count] = malloc(sizeof(char));
		//messages[msg_count] = (char *) str;
		msg_count++;
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

	sleep(111);

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


 	printf("Ticket Office nº %2d: Created\n", *(int*)threadId); 
	
	//receiving the message
	while(1) {
		if(msg_count != 0) {
			
			/*struct client_args_t reservation;
			read_msg(messages[msg_count], &reservation);
			printf("Bilheteira %i leu: %s\n", *(int*) threadId, messages[msg_count]);
			msg_count--;*/
		}
		sleep(1);
	}
	
  	

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