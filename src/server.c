#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

// $ server <num_room_seats> <num_ticket_offices> <open_time>

struct server_args_t {
	int num_room_seats;
	int num_ticket_offices;
	int open_time;
};

void parse_args(char *argv[], struct server_args_t * args);
void print_args(struct server_args_t * args);

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
  sleep(1);

  return 0;
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