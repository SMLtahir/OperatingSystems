#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <sys/fcntl.h>
//Definitions
#define BUF_SIZE 4096
struct client_data{
	struct client_data *next;
	char *buf;
	int to_delete;
	int con_num;
	int sock_id;
	int return_val;
	struct timeval start,end;
};

//variables for timing
unsigned long connection_count = 0,active_count = 0;
unsigned long avg_time_count = 0;
double avg_time = 0.0;
//varialbes for print
int print_flag = 0;
int print_level = 10;
//Function declarations
struct client_data * insert_data(struct client_data * Head);
struct client_data * delete_node(struct client_data * Head);

int main(int argc, char *argv[])
{
	int opt = 1;	
	int sock, addrlen, new_socket, newsockfd, i, valread, sd,portno;
	int max_sd;
	struct sockaddr_in address;
	struct client_data *HEAD,*blk;
	if(argc < 2)
	{
		print_level = 10;
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

	//set of socket descriptors
	fd_set readfds;

	//create a master socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		printf("socket failed");
		exit(1);
	}

	/*if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
	{
		printf("setsockopt");
		exit(1);
	}*/

	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(portno);

	//bind the socket
	if (bind(sock, (struct sockaddr *)&address, sizeof(address))<0)
	{
		perror("bind failed");
		exit(1);
	}

	
	listen(sock, SOMAXCONN);
	printf("Listening on port %d\n",portno);
	printf("Will print avg time for approx every %d client connections\n",print_level);

	//accept the incoming connection
	addrlen = sizeof(address);
	HEAD = NULL;
	while (1)
	{
		//clear the socket set
		FD_ZERO(&readfds);

		//add master socket to set
		FD_SET(sock, &readfds);
		max_sd = sock;
		for (blk=HEAD;blk;blk = blk->next)
		{
			//socket descriptor
			sd = blk->sock_id;
			
			//if valid socket descriptor then add to read list
			if (sd > 0){
				FD_SET(sd, &readfds);
				//printf("Registering socket %d\n",sd);		
			}
			if (sd > max_sd)
				max_sd = sd;
		}


		//wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
		newsockfd = select(max_sd + 1, &readfds, NULL, NULL, NULL);

		if ((newsockfd < 0) && (errno != EINTR))
		{
			printf("select error, newsockfd = %d,err = %s\n",newsockfd,strerror(errno));
		}
		
		
		
		if (FD_ISSET(sock, &readfds))
		{
			if ((new_socket = accept(sock, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
			{
				perror("accept");
				exit(EXIT_FAILURE);
			}
			fcntl(new_socket, F_SETFL, O_ASYNC);

			connection_count++;
			active_count++;
			//printf("Total connections = %d,Active conections = %d\n",connection_count,active_count);
                  	//printf("connection number = %d, socket id = %d\n",connection_count,new_socket);
			//getch();
		  	//Create a linked list data
			//printf("Calling insert into head\n");
			HEAD = insert_data(HEAD);
			HEAD->sock_id = new_socket;
			HEAD->buf = (char *)malloc(sizeof(char) * BUF_SIZE);
			gettimeofday(&HEAD->start,NULL);
		}

		//else its some IO operation on some other socket :)
		for(blk=HEAD;blk;blk = blk->next)
		{			

			if (FD_ISSET( blk->sock_id, &readfds))
			{							
								
				blk->return_val = read(blk->sock_id, blk->buf, BUF_SIZE);			
				if(blk->return_val > 0)
				{
					//printf("Read something\n");					
                    			blk->return_val = read(sd, blk->buf, BUF_SIZE);
				}
                		else if(blk->return_val == 0 )
                		{
					//printf("Read everything\n");		                    			
					blk->to_delete = 1;
                    			active_count--;
                    			gettimeofday(&blk->end,NULL);
                    			close(blk->sock_id);
                		}
                		else if(blk->return_val == -1)
               			 {
                 			printf("Unable to start read\n");
					printf("status of connection %d is %s\n",blk->con_num,strerror(errno));
                 			exit(0);
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

	return 0;
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
				free((void *)temp->buf);
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
			free((void *)temp->buf);
			free(temp);
		//	printf("A node deleted\n");
		}
	}
	return Head;
}
