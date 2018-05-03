#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
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
  sleep(1);

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