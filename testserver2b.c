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
struct client_data{
	struct client_data *next;
	int to_delete;
	long con_num;
	int sock_id;
	char *buffer;
	int return_val;
	struct timeval start,end;
};
unsigned long connection_count = 0,active_count = 0;
unsigned long avg_time_count = 0;
double avg_time = 0.0;
#define BUF_SIZE 4096
//struct aiocb *aiocbo_ptr[10000];
int print_flag = 0;
void executeFunction(int newsockfd);
void callAIOREAD(struct client_data* mydata);
struct client_data * insert_data(struct client_data * Head);
struct client_data * delete_node(struct client_data * Head);
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno, clilen, *new_sock;
  	char *buffer;
  	int i;
	struct sockaddr_in serv_addr, cli_addr;
	struct client_data *HEAD,*blk,*debug;
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
	HEAD = NULL;
	/* Accept actual connection from the client */
	while (1)
	{
		newsockfd=-1;
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
	//	printf("Value of newsockfd = %d\n",newsockfd);
		if (newsockfd>0 )
		{
                  	//printf("connection number %d\n",i);
			connection_count++;
			active_count++;
			//printf("Total connections = %d,Active conections = %d\n",connection_count,active_count);
                  	//printf("connection number %d\n",connection_count);
		  	//Create a linked list data
			//printf("Calling insert into head\n");
			HEAD = insert_data(HEAD);
			HEAD->sock_id = newsockfd;
			//printf("Inserted into data list\n");
			gettimeofday(&HEAD->start,NULL);
			callAIOREAD(HEAD);
		}
		//If there are no more connections to satisfy
		if(newsockfd < 0)
		{
			for(blk=HEAD;blk;blk = blk->next)
 			{
				//printf("status of connection %d is %s\n",blk->con_num,strerror(aio_error(blk->aiocbo_ptr)));
				if(blk->return_val > 0)
				{

                    free((void *)blk->buffer);
                    callAIOREAD(blk);
				}
                else if(blk->return_val == 0)
                {
                    blk->to_delete = 1;
                    active_count--;
                    //printf("\nTotal connections = %d,Active conections = %d\n",connection_count,active_count);
                    //printf("Connection number %d set for deletion\n",blk->con_num);
                    gettimeofday(&blk->end,NULL);
                    close(blk->sock_id);
                }
                else if(blk->return_val == -1)
                {
                    //printf("status of connection %ld is %s\n",blk->con_num,strerror(blk->return_val));
                    //callAIOREAD(blk,blk->sock_id );
                    printf("Unable to start read\n");
                    blk->to_delete = 1;
                    close(blk->sock_id);
                    break;
                }

			}
			/*if(active_count == 0 && connection_count > 0)
			{*/
				HEAD = delete_node(HEAD);
			//}
			if(print_flag == 1 /*&& (connection_count % 1) == 0*/)
			{
				printf("avg time for %ld clients = %lf\n",avg_time_count,avg_time);
				print_flag = 0;
			}
		}
	}

	return 0;
}

void callAIOREAD(struct client_data* mydata){
	mydata->buffer = (char *)malloc(sizeof(char) * BUF_SIZE);
	//printf("status of connection %ld is %s\n",mydata->con_num,strerror(read(sockfd,(void*)mydata->buffer,BUF_SIZE)));
    mydata->return_val = read(mydata->sock_id,(void*)mydata->buffer,BUF_SIZE);
if (mydata->return_val == -1)
	{
			printf("Unable to start read\n");
			printf("status of connection %ld is %s\n",mydata->con_num,strerror(errno));
	}
}
struct client_data * insert_data(struct client_data * Head)
{
	struct client_data * temp;
	temp = (struct client_data *)malloc(sizeof(struct client_data));
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
				//printf("Deleting node %d\n",temp->con_num);
				//connection_count--;
			//	close(temp->sock_id);
				//printf("Start and end time = %ld,%ld",temp->start.tv_sec,temp->end.tv_sec);
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

				//printf("time for client %ld = %ld\n",temp->con_num,((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec)));
				//printf("avg time for %ld clients = %ld\n",avg_time_count,avg_time);
				print_flag = 1;
				free((void *)temp->buffer);
				free(temp);
				//printf("A node deleted\n");

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
			//printf("Deleting node %d\n",temp->con_num);
			//connection_count--;
		//	close(temp->sock_id);
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
			//printf("time for client %ld = %ld\n",temp->con_num,((temp->end.tv_sec * 1000000 + temp->end.tv_usec)-(temp->start.tv_sec * 1000000 + temp->start.tv_usec)));
			//printf("avg time for %ld clients = %ld\n",avg_time_count,avg_time);
			free((void *)temp->buffer);
			free(temp);
		//	printf("A node deleted\n");
		}
	}
	return Head;
}