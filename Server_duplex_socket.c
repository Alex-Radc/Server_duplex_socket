#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>
#include <curses.h>
#include <fcntl.h>


#define MAX_BUF_LEN 128
#define SIZE 10
#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD   -12
#define SUCCESS        0


void error_print(char *ptr)
{
	perror(ptr);
	exit(EXIT_FAILURE);
}
////////////////////////////////////////////////////////////////////////////
void quit_tranmission(int sig)
{
	printf("recv a quit signal = %d\n", sig);
	exit(EXIT_SUCCESS);
}
///////////////////////////////////////////////////////////////////////
void create_server(void *a )
{
	int sockfd = 0;
	int enable = 1;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);/*IPV4 streaming protocol is TCP protocol*/
	if (sockfd < 0)
		error_print("socket");
	//struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;/*IPV4*/
	servaddr.sin_port = 1334 + (int)a;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");/*Use the local loopback address for testing*/
	/*inet_aton("127.0.0.1",&servaddr.sin_addr);//Same function as inet_addr*/

	/*setsockopt ensures that the server can restart the server without waiting for the end of the TIME_WAIT state, and continue to use the original port number*/
	//////////////////////////////////////////////////////////////////////////
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
	 perror("setsockopt(SO_REUSEADDR) failed");
	////////////////////////////////////////////////////////////////////////////
	/*Bind local Socket address*/
	if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
		error_print("bind ");

	/*Monitor connection*/
	if (listen(sockfd, SOMAXCONN) < 0)
		error_print("listen");

	struct sockaddr_in peeraddr;/*Store the client Socket information of a successful connection*/
	socklen_t peerlen = sizeof(peeraddr);
	int conn;
	/*Receive the first connection request from the listening queue*/
	if ((conn = accept(sockfd, (struct sockaddr*) &peeraddr, &peerlen)) < 0)
		error_print("accept");
	pid_t pid;

	pid = fork();/*Create a new child process*/
	if (pid == -1)
	{
		error_print("fork");
	}
	if (pid == 0)
	{/*Used in the child process to send data to the client*/
		signal(SIGUSR1, quit_tranmission);/*Callback function to handle communication interruption*/
		char send_buf[MAX_BUF_LEN] = { 0 };
		/*If the client Ctrl+C ends the communication process, what fgets gets is NULL, otherwise it enters the loop to send data normally*/
		while (fgets(send_buf, sizeof(send_buf), stdin) != NULL)
		{
			write(conn, send_buf, strlen(send_buf));
			bzero(send_buf, strlen(send_buf));/*Clear the sending buffer after sending*/
		}
		exit(EXIT_SUCCESS);/*Exit the child process successfully*/
	}
	else
	{
		char recv_buf[MAX_BUF_LEN] = { 0 };
		while (1)
		{
			bzero(recv_buf, strlen(recv_buf));
			int ret = read(conn, recv_buf, sizeof(recv_buf));/*Read data sent by conn connection*/
			if (ret < 0)
				error_print("read");
			else if (ret == 0)
			{
				printf("client is close!\n");
				break; //The parent process receives the server-side exit information (the server Ctrl+C ends the communication process, the read function returns a value of 0, and the loop exits)
			}
			fputs(recv_buf, stdout);
		}
		kill(pid, SIGUSR1);/*When the parent process ends, a signal must be sent to the child process to tell the child process to terminate the reception, otherwise the child process will always wait for input*/
	}
	close(conn);
	close(sockfd);
}
////////////////////////////////////////////////////////////////////
int main(void)
{
	int i = 0;
	pthread_t thread[SIZE];
	int status[SIZE] = {0};
	int status_addr = 0;

	for(i = 0; i < SIZE; i++)
	{
		status[i] = pthread_create(&thread[i], NULL, (void *)create_server, (void *)i);
		if (status[i] != 0)
		{
			printf("main error: can't create thread, status = %d\n", status[i]);
			exit(ERROR_CREATE_THREAD);
		}
		status[i] = pthread_join(thread[i], (void**)&status_addr);
		if (status[i] != SUCCESS)
		{
			printf("main error: can't join thread, status = %d\n", status[i]);
			exit(ERROR_JOIN_THREAD);
		}
	}
	return 0;
}
