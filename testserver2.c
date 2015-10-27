#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <aio.h>
#include<errno.h>
//Definitions
#define BUF_SIZE 4096
//Client data to be stored in a linked list
struct client_data{
	struct aiocb *aiocbo_ptr;
	struct client_data *next;
	int to_delete;
	long con_num;
	int sock_id;
	struct timeval start,end;
};
//variables for timing
unsigned long connection_count = 0,active_count = 0;
unsigned long avg_time_count = 0;
double avg_time = 0.0;
//varialbes for print
int print_flag = 0;
int print_level = 1000;
//Function declarations
void callAIOREAD(struct aiocb* aiocbptr, int offset);
struct client_data * insert_data(struct client_data * Head);
struct client_data * delete_node(struct client_data * Head);

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen;
  	char *buffer;
  	int i;
	struct sockaddr_in serv_addr, cli_addr;
	struct client_data *HEAD,*blk;
	
	if(argc < 2)
	{
		print_level = 1000;
		portno = 5555;
	}
	else if(argc == 2)
	{		
		print_level = 1000;
		portno = (int)atoi(argv[1]);
	}
	else if(argc == 3)
	{
		print_level = (int)atoi(argv[2]);
		portno = (int)atoi(argv[1]);
	}
	else
	{
		printf("Invalid number of arguments\n");
	}

	/* First call to socket() function */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if (sockfd < 0) {
		printf("ERROR opening socket");
		exit(1);
	}

	/* Initialize socket structure */
	memset((char *)&serv_addr,0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	/* Now bind the host address using bind() call.*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR on binding");
		exit(1);
	}

	listen(sockfd, SOMAXCONN);
	clilen = sizeof(cli_addr);
	 printf("Listening on port %d\n",portno);
	printf("Will print avg time for approx every %d client connections\n",print_level);

	HEAD = NULL;
	/* Accept actual connection from the client */
	while (1)
	{
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if (newsockfd>0 )
		{
			connection_count++;
			active_count++;
		  	//Create a linked list data
			HEAD = insert_data(HEAD);
			HEAD->sock_id = newsockfd;
			gettimeofday(&HEAD->start,NULL);
			memset(HEAD->aiocbo_ptr, 0, sizeof(struct aiocb));
			HEAD->aiocbo_ptr->aio_buf = (char *)malloc(sizeof(char) * BUF_SIZE);
			HEAD->aiocbo_ptr->aio_nbytes = BUF_SIZE;
			HEAD->aiocbo_ptr->aio_fildes = newsockfd;
			callAIOREAD(HEAD->aiocbo_ptr, 0);
		}
		//If there are no more connections to satisfy
		if(newsockfd < 0)
		{
			for(blk=HEAD;blk;blk = blk->next)
 			{
				if(aio_error(blk->aiocbo_ptr) != EINPROGRESS)
				{

					if(aio_error(blk->aiocbo_ptr) == 0)
				 	{
						if(aio_return(blk->aiocbo_ptr) > 0)
						{
							//free((void *)blk->aiocbo_ptr->aio_buf);
							callAIOREAD(blk->aiocbo_ptr, ((blk->aiocbo_ptr->aio_offset)+BUF_SIZE));
						}
						else if(aio_return(blk->aiocbo_ptr) == 0)
						{
							//transfer complete delete node
							blk->to_delete = 1;
							active_count--;
							gettimeofday(&blk->end,NULL);
							close(blk->sock_id);
						}
						else
						{
							printf("error in aio_return connection %ld terminated with%s\n",blk->con_num,strerror(aio_error(blk->aiocbo_ptr)));
						}
					}
					else
					{
						printf("error in aio_read connection %ld terminated with %s\n",blk->con_num,strerror(aio_error(blk->aiocbo_ptr)));
					}
				}

			}
			HEAD = delete_node(HEAD);
			if(print_flag == 1 && (connection_count % print_level) == 0)
			{
				printf("avg time for %ld clients = %lf\n",avg_time_count,avg_time);
				print_flag = 0;
			}
		}
	}

	return 0;
}

void callAIOREAD(struct aiocb* aiocbptr, int offset){
	aiocbptr->aio_offset = offset;
if (aio_read(aiocbptr) == -1)
	{
			printf("Unable to start read");
	}
}
struct client_data * insert_data(struct client_data * Head)
{
	struct client_data * temp;
	temp = (struct client_data *)malloc(sizeof(struct client_data));
	temp->aiocbo_ptr = (struct aiocb *)malloc(sizeof(struct aiocb));
	temp->next = Head;
	temp->to_delete = 0;
	temp->con_num = connection_count;
	return temp;
}
struct client_data * delete_node(struct client_data * Head)
{
	struct client_data *temp,*curr_data;
	if(Head !=NULL)
	{
		curr_data = Head;
		while(curr_data->next != NULL)
		{
			if(curr_data->next->to_delete == 1)
			{

				temp = curr_data->next;
				curr_data->next = curr_data->next->next;
				if(avg_time_count == 0)
				{
					avg_time_count++;
					avg_time = (double)((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec));
				}
				else
				{
					avg_time_count++;
					avg_time = (double)((avg_time * (avg_time_count -1)) + ((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec))) / avg_time_count;
				}

				print_flag = 1;
				free((void *)temp->aiocbo_ptr->aio_buf);
				free(temp->aiocbo_ptr);
				free(temp);

			}
			else
			{
				curr_data = curr_data->next;
			}
		}

		if(Head->to_delete == 1)
		{
			temp = Head;
			Head = Head->next;
			if(avg_time_count == 0)
			{
				avg_time_count++;
				avg_time = (double)((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec));
			}
			else
			{
				avg_time_count++;
				avg_time = (double)((avg_time * (avg_time_count -1)) + ((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec))) / avg_time_count;
			}
			print_flag = 1;
			free((void *)temp->aiocbo_ptr->aio_buf);
			free(temp->aiocbo_ptr);
			free(temp);
		}
	}
	return Head;
}
