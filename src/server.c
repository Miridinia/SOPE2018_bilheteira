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
#include <time.h>

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
void initSeats(int num);
void delay(unsigned int mseconds);
void answerClient(int pid, char *message, int respostaDada);

int isSeatFree(Seat *seats, int seatNum);
void bookSeat(Seat *seats, int seatNum, int clientId);
void freeSeat(Seat *seats, int seatNum);

//Globals
int ALARM_ON = 0;
Seat seats;
int conditionMet = 0;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char *message;

static void ALARMhandler(int sig)
{
	printf("Ticket Office Closed!\n");
	
	ALARM_ON = 1;
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
	initSeats(seats.num_room_seats);

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
	siginterrupt(SIGALRM, 1);
	int fd = openFIFO(FIFO_NAME_CONNECTION, O_RDONLY);
	sleep(1); //to wait threads for doing something??

	int check = 0;
	res = pthread_mutex_lock(&mutex); //making everything sleep??
	checkResult("pthread_mutex_lock()\n", res);
	while(!ALARM_ON) {

		char str[100];
		if(readOnFIFO(fd, str) && check == 0) {
			check = 1;
			printf("%s \n", str);
		}
		if(check && !ALARM_ON) {
			message = str;
			conditionMet = 1;
			while(conditionMet != 0 && !ALARM_ON){
				res = pthread_cond_broadcast(&cond); //send everyone a message
				checkResult("pthread_cond_broadcast()\n", res);
				res = pthread_mutex_unlock(&mutex);
				checkResult("pthread_mutex_unlock()\n", res);
				if(conditionMet != 0) {
					res = pthread_mutex_lock(&mutex); //making everything sleep??
					checkResult("pthread_mutex_lock()\n", res);
				}
				//printf("esperando...\n");
			}
			check = 0;
		}

	}

	res = pthread_cond_broadcast(&cond);

	for(t=1; t<= args.num_ticket_offices; t++){
		thrArg[t-1] = t;
		pthread_join(tid[t-1],NULL);
		res = pthread_cond_broadcast(&cond);
		sleep(1);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(1);
		}
	}

	res = pthread_mutex_unlock(&mutex);
	checkResult("pthread_mutex_unlock()\n", res);
	res = pthread_cond_destroy(&cond);
	checkResult("pthread_cond_destroy()\n", res);
	res = pthread_mutex_destroy(&mutex);
	checkResult("pthread_mutex_destroy()\n", res);
	closeFIFO(fd);
	killFIFO(FIFO_NAME_CONNECTION);

	return 0;
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

 	char ans[MAX_TOKEN_LEN];

	//receiving the message
	int lol = 0;
	int respostaDada = 0;
	while(!ALARM_ON) {
		printf("Thread %d blocked because condition is not met\n", threadNum);
		res = pthread_mutex_lock(&mutex);
		checkResult("pthread_mutex_lock()\n", res);
		res = pthread_cond_wait(&cond, &mutex);
		checkResult("pthread_cond_wait()\n", res);
		if(conditionMet == 1) {
			//ZONA CRíTICA!!!!!!
			printf("Thread %d executing critical section for 1 seconds ...\n",threadNum);
			res = pthread_mutex_unlock(&mutex);
			checkResult("pthread_mutex_unlock()\n", res);

			res = pthread_mutex_lock(&mutex);
			checkResult("pthread_mutex_lock()\n", res);
			conditionMet = 0;
			struct client_args_t data;
			read_msg(message, &data);

			if(data.num_wanted_seats> MAX_CLI_SEATS || data.num_wanted_seats<1){
									//enviar resposta cliente -1
									printf("\n\n------ERROR -1-------\n");
									sprintf(ans, "%d", -1);
									answerClient(data.pid, ans, respostaDada);
									respostaDada = 1;
			}
			if(data.num_wanted_seats> data.num_pref_seats || data.num_wanted_seats<1){
									//enviar resposta cliente -4
									printf("\n\n------ERROR -4--------- \n");
									sprintf(ans, "%d", -4);
									answerClient(data.pid, ans, respostaDada);
									respostaDada = 1;
			}

			int indexSeats=0;
			int total=0;

			int done=0; //bool
			int reserveDone=0; //bool

			int *reserveSeats = malloc(sizeof(int) * data.num_wanted_seats);

			while(!done && indexSeats<data.num_pref_seats){
					printf("Index: %i\n",indexSeats);
					printf("Data pref seats: %i\n",data.num_pref_seats);
					printf("Data wanted seats: %i\n",data.num_wanted_seats);
					printf("Seats Numero de room seats: %i\n",seats.num_room_seats);
					printf("Current Seat: %i\n",data.pref_seat_list[indexSeats]);


					if(data.pref_seat_list[indexSeats]> seats.num_room_seats || data.pref_seat_list[indexSeats]<1){
						//enviar resposta cliente -3
						printf("\n\n------ERROR -3--------- \n");
						sprintf(ans, "%d", -3);
						answerClient(data.pid, ans, respostaDada);
						respostaDada = 1;
					}

					if(isSeatFree(&seats , data.pref_seat_list[indexSeats])==0){
						bookSeat(&seats,data.pref_seat_list[indexSeats],data.pid);
						reserveSeats[total]=data.pref_seat_list[indexSeats];
						printf("Atendeu lugar: %i\n",data.pref_seat_list[indexSeats]);
						total++;
					}
					else{
						printf("Lugar %i ocupado\n",data.pref_seat_list[indexSeats]);
					}

					indexSeats++;
					if(total == data.num_wanted_seats){
						reserveDone=1;
						done=1;
					}

			}
			if(reserveDone){
				int n;
				char aux[MAX_TOKEN_LEN];
				sprintf(ans, "%d", total);
				strcat(ans, " ");
				for(n=0;n<total;n++){
					sprintf(aux, "%d", reserveSeats[n]);
					strcat(ans, aux);
					strcat(ans, " ");					
					printf("RESERVOU: %i\n",reserveSeats[n]);
					//enviar mensagem de sucesso cliente
				}
				answerClient(data.pid, ans, respostaDada);
					printf("\n\n");
			}
			else{
				printf("NAO RESERVOU\n");
				if(total== 0){
					//enviar resposta cliente -6
					printf("\n\n------ERROR -6--------- \n");
					sprintf(ans, "%d", -6);
					answerClient(data.pid, ans, respostaDada);
					respostaDada = 1;
				}
				else{
					//enviar resposta cliente -5
					printf("\n\n------ERROR -5--------- \n");
					sprintf(ans, "%d", -5);
					answerClient(data.pid, ans, respostaDada);
					respostaDada = 1;
					int ni;
					for(ni=0;ni<total;ni++){
					printf("Lugar %i reservado, irá ser apagado\n",reserveSeats[ni]);
					freeSeat(&seats,reserveSeats[ni]);
				}
				}
					printf("\n\n");
			}
			indexSeats=0;
			total=0;

			done=0; //bool
			reserveDone=0; //bool

			free(reserveSeats);

			lol = 1;
			res = pthread_mutex_unlock(&mutex);
			checkResult("pthread_mutex_unlock()\n", res);
			//FIM ZONA CRÍTICA!!!
			printf("\n\n*************RECEBEU**********************\n");
			printf("bilheteira: %i, recebeu %i\n", threadNum, data.pid);
			printf("******************************************\n\n\n");

			
			printf("CLIENTE PROCESSADO \n");


		}
		if(lol == 1){
			lol = 0;
		}	else {
			res = pthread_mutex_unlock(&mutex);
			checkResult("pthread_mutex_unlock()\n", res);
		}
		DELAY(5);
		respostaDada = 0;
	}

	printf("FECHEI %d! \n", threadNum);

	return NULL;
}

void answerClient(int pid, char *message, int respostaDada) {
	if(!respostaDada) {
		char name[MAX_TOKEN_LEN];
		char try[MAX_TOKEN_LEN];
		name[0] = '\0';
		try[0] = '\0';
		strcat(try,"%0");
		char width_pid[10];
		sprintf(width_pid, "%d", WIDTH_PID);
		strcat(try, width_pid);
		strcat(try, "d");
		sprintf(name, try, pid);
		char fifoName[MAX_TOKEN_LEN];
		fifoName[0] = '\0';
		strcat(fifoName, "ans");
		strcat(fifoName, name);

		printf("%s\n", fifoName);
		
		int msglen = strlen(message)+1;

		int fd = openFIFO(fifoName, O_WRONLY);
		writeOnFIFO(fd, message, msglen);

		printf("RESPOTA DADA: %s ao: %s \n", message, name);
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
		killFIFO(FIFO_NAME_CONNECTION);
		exit(0);
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
	//struct sigaction int_handler = {.sa_handler=ALARMhandler};
  	//sigaction(SIGALRM,&int_handler,1);
  	siginterrupt(SIGALRM, 1);
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

void initSeats(int num){ //inicializar a 0 o array com num de comprimento
	seats.seats_taken = malloc(sizeof(int) * num);
	memset(seats.seats_taken, 0, sizeof(int)*num);

}

int isSeatFree(Seat *seats, int seatNum){//caso esteja livre o valor que aparece é 0, se estiver ocupado aparece o clienteId
	if(seats->seats_taken[seatNum]==0){
		return 0;
	}
	else return seats->seats_taken[seatNum];
	delay(1);
}

void bookSeat(Seat *seats, int seatNum, int clientId){
	
	seats->seats_taken[seatNum]=clientId;
	delay(1);
}

void freeSeat(Seat *seats, int seatNum){
	seats->seats_taken[seatNum]=0;
	delay(1);
}

void delay(unsigned int mseconds)
{
    sleep(mseconds);
}

