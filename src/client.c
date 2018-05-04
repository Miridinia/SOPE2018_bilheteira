#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h> 

#include "ticket.h"

// $ client <time_out> <num_wanted_seats> <pref_seat_list>
// client 120 3 "11 12 13 14 15"

struct client_args_t {
	int time_out;
	int num_wanted_seats;
	int pref_seat_list [MAX_CLI_SEATS];
	int num_pref_seats;
};

void parse_args(char *argv[], struct client_args_t * args);
void print_args(struct client_args_t * args);
void createFIFO(const char *pathname);
int openFIFO(const char *pathname, mode_t mode);
void writeOnFIFO(int fd, char *message, int messagelen);
void closeFIFO(int fd);
void killFIFO(char *pathname);

int main(int argc, char *argv[]) {
  printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

  if (argc == 4) {
    printf("ARGS: %s | %s | %s\n", argv[1], argv[2], argv[3]);
	} else {
		return -1;
	}

	struct client_args_t args; 

	parse_args(argv, &args);
	print_args(&args);

	//Here only will create the fifo for is PID client
	//The requests fifos will be created by the server
	int fd = openFIFO(FIFO_NAME_CONNECTION, O_WRONLY);
	char *msg = "Olá Povo!\0";
	int msglen = strlen(msg)+1;
	writeOnFIFO(fd, msg, msglen);
	closeFIFO(fd);
  return 0;
}

void parse_args(char* argv[], struct client_args_t * args) {
	sscanf(argv[1], "%d", &(args->time_out));
	sscanf(argv[2], "%d", &(args->num_wanted_seats));
	args->num_pref_seats = 0;
	char* next_seat = strtok(argv[3], " ");
	while(next_seat != NULL) {
		sscanf(next_seat, "%d", &(args->pref_seat_list[args->num_pref_seats]));
		args->num_pref_seats++;
		next_seat = strtok(NULL, " ");
	}
}

void print_args(struct client_args_t * args) {
	printf("Time Out: %d\n", args->time_out);
	printf("Number of Wanted Seats: %d\n", args->num_wanted_seats);
	printf("Preferred Seats:");
	for(size_t i = 0; i < args->num_pref_seats; i++) {
		printf(" %d", args->pref_seat_list[i]);
	}
	printf("\n");
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
	printf("ENVIOU: %s\n", message);
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