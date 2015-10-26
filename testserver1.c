#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <pthread.h>
void *executeFunction(void *thread_struct);
struct thread_data{
	int sock;
	char *buffer_data;
	struct timeval start,end;
	};
#define BUF_SIZE 4096
pthread_mutex_t mutextime;
unsigned long avg_time_count = 0;
unsigned long connection_count = 0;
double avg_time = 0.0;
int print_flag = 0;
int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno, clilen, *new_sock;
	pthread_t threads[10000];
	struct sockaddr_in serv_addr, cli_addr;
	struct thread_data * thread_dataptr;
	pthread_attr_t attr;
	int i=0;
	/* Initialize and set thread detached attribute */
   	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	bzero((char *)&serv_addr, sizeof(serv_addr));
	portno = 5555;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	/* Accept actual connection from the client */
	while (1) {
		
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

		if(newsockfd > 0)
		{		
			pthread_t t;
			thread_dataptr = (struct thread_data*)malloc(sizeof(struct thread_data));
			thread_dataptr->buffer_data = (char *)malloc(sizeof(char)*BUF_SIZE);
			thread_dataptr->sock = newsockfd;

			if (pthread_create(&t, &attr, executeFunction, (void *)thread_dataptr) < 0)
			{
				perror("could not create thread");
				return 1;
			}
			if(print_flag == 1 && (connection_count % 1000) == 0)
			{
				printf("avg time for %ld clients = %lf\n",avg_time_count,avg_time);
				print_flag = 0;
			}
			if(i<10000)
			i++;
			else
			i=0;
		}
	}
	pthread_attr_destroy(&attr);
	pthread_exit(NULL);

	return 0;
}

void *executeFunction(void *thread_struct) {
	
	// Start timing here
	struct thread_data * my_data;
	int n = 0;
	my_data = (struct thread_data *)thread_struct;
	double mytime;
	connection_count++;
	gettimeofday(&my_data->start,NULL);	
	bzero(my_data->buffer_data,BUF_SIZE);

      while( (n = recv(my_data->sock ,my_data->buffer_data  , BUF_SIZE , 0))>0)
      {

      }
	if (n < 0) {
		perror("ERROR reading from socket");
		exit(1);
	}
	close(my_data->sock);
	gettimeofday(&my_data->end,NULL);
	mytime = (double)((my_data->end.tv_sec * 1000000 + my_data->end.tv_usec)-(my_data->start.tv_sec * 1000000 + my_data->start.tv_usec));
	pthread_mutex_lock (&mutextime);
	if(avg_time_count == 0)
	{
		avg_time_count++;
		avg_time = mytime;
	}
	else
	{
		avg_time_count++;
		avg_time = (double)((avg_time * (avg_time_count -1)) + mytime) / avg_time_count;
	}
	pthread_mutex_unlock (&mutextime);
	print_flag = 1;	
	free(my_data->buffer_data);
	free(my_data);
	
	pthread_exit((void *)0); 

}

