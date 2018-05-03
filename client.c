#include <stdio.h>

#include <sys/types.h>
#include <unistd.h>

// $ client <time_out> <num_wanted_seats> <pref_seat_list>
// client 120 3 "11 12 13 14 15"

int parseSeats(char* seats);

int main(int argc, char *argv[]) {
  printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

  if (argc == 4)
    printf("ARGS: %s | %s | %s\n", argv[1], argv[2], argv[3]);

	parseSeats(argv[3]);
	printf("%li", sizeof(argv[3])/sizeof(char));
  sleep(1);

  return 0;
}

int parseSeats(char* seats) {
	int seat[sizeof(seats)/8];
	size_t i;
	for(i=0; i<sizeof(seats);i++){
		if(seats[i] != " ")
			printf("%i", (int) seats[i]);
	}
	
	return seat;
}
