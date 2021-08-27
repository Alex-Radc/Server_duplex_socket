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
#include <sys/time.h>


#define MAX_BUF_LEN 500
#define SIZE 10
#define ERROR_CREATE_THREAD -11
#define ERROR_JOIN_THREAD   -12
#define SUCCESS   0
#define ERROR  1
#define PORT 6490
#define MYPORT 6950

void create_server(char *recv_buf);
int initial_receive(int *conn,int *sockfd) ;
int initial_send(int *conn, int *sockfd, int client_id);
void send_to_client(int id, int sockfd, char *recv_buf);
int receive_from_client(int conn, int sockfd);
int parser(char *recv_buf);

int temp = 0;
//////////////////////////////////////////////////////////////////
int main(void)
{
	int sockfd = 0 ;
	int conn = 0;

	// receive information
	initial_receive(&conn,&sockfd);
	receive_from_client(conn, sockfd);

	return 0 ;
}
/////////////////////////////////////////////////////////////////////////////
void error_print(char *ptr)
{
	perror(ptr);
	exit(EXIT_FAILURE);
}
////////////////////////////////////////////////////////////////////////////
void quit_tranmission(int sig)
{
	//printf("recv a quit signal = %d\n", sig);
	exit(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////
int initial_receive(int *conn,int *sockfd)
{
	int enable = 1;
	struct sockaddr_in servaddr;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);/*IPV4 streaming protocol is TCP protocol*/
	if (*sockfd < 0)
		error_print("socket");
	//struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr)) ;
	servaddr.sin_family = AF_INET;/*IPV4*/
	servaddr.sin_port = PORT;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");/*Use the local loopback address for testing*/
	/*inet_aton("127.0.0.1",&servaddr.sin_addr);//Same function as inet_addr*/

	/*setsockopt ensures that the server can restart the server without waiting for the end of the TIME_WAIT state, and continue to use the original port number*/
	//////////////////////////////////////////////////////////////////////////
	if(setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");
	////////////////////////////////////////////////////////////////////////////
	/*Bind local Socket address*/
	if (bind(*sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0)
		error_print("bind ");

	/*Monitor connection*/	//int sockfd = 0;
	if (listen(*sockfd, SIZE) < 0)  //SOMAXCONN
		error_print("listen");

	struct sockaddr_in peeraddr;/*Store the client Socket information of a successful connection*/
	socklen_t peerlen = sizeof(peeraddr);

	/*Receive the first connection request from the listening queue*/
	if ((*conn = accept(*sockfd, (struct sockaddr*) &peeraddr, &peerlen)) < 0)
		error_print("accept");
	printf("work serv\n");
	return *sockfd;
}
/////////////////////////////////////////////////////////////////
int receive_from_client(int conn, int sockfd)
{
	int id = 0;
	char recv_buf[MAX_BUF_LEN] = { 0 };
	while (1)
	{
		int ret = read(conn, recv_buf, MAX_BUF_LEN);//Read data sent by conn connection
		if (ret < 0)
			error_print("read");
		else if (ret == 0)
		{
			printf("client is close!\n");
			break; //The parent process receives the server-side exit information (the server Ctrl+C ends the communication process, the read function returns a value of 0, and the loop exits)
		}
		char *r_buff =(char*)malloc(sizeof(recv_buf)+1);
		strncpy(r_buff,recv_buf,sizeof(recv_buf)+1);
		id = parser(recv_buf);
		{
			printf("Resv_buf serv = %s\n",recv_buf);
			pthread_t thread;
			int status = 0;
			status = pthread_create(&thread, NULL, (void *)create_server, (void*) r_buff);
			if (status != 0)
			{
				printf("main error: can't create thread, status = %d\n", status);
				exit(ERROR_CREATE_THREAD);
			}
		}
		bzero(recv_buf, MAX_BUF_LEN);
	}
	//kill(pid, SIGUSR1);/*When the parent process ends, a signal must be sent to the child process to tell the child process to terminate the reception, otherwise the child process will always wait for input

	close(conn);
	close(sockfd) ;
	return 0;
}
///////////////////////////////////////////////////////////////////////
void create_server(char *recv_buf)
{
	int sockfd = 0;
	int conn = 0;
	int client_id = 0;

	client_id = parser(recv_buf);
	initial_send(&conn, &sockfd, client_id);
	send_to_client(client_id, sockfd, recv_buf);
	temp++;
	//printf("temp = %d\n", temp);
}
///////////////////////////////////////////////////////////////////
int initial_send(int *conn, int *sockfd, int client_id)
{
	printf("initial_send()\n");
	struct sockaddr_in servaddr;
	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd < 0)
		error_print("socket");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = MYPORT + client_id;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if ((*conn = connect(*sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr))) < 0)
		error_print("connect");
	if (*sockfd <= 2)
	{
		printf("Error connect\n");
		return ERROR;
	}
	return SUCCESS;
}
////////////////////////////////////////////////////////////////////
void send_to_client(int id, int sockfd, char *server_message )
{
	int i = 0;
	struct timespec mt2;
	int set = -1;
	char client_message[MAX_BUF_LEN] = {0};
	if ((3 > sockfd) ||(NULL == server_message))
	{
		printf("invalid input param\n");
		return;
	}
	for(i = 0; i < SIZE; i++)
	{
		char buf0[20] = {'\0'};
		char buf1[20] = {'\0'};
		char buf2[20] = {'\0'};
		char buf3[20] = {'\0'};
		char buf4[20] = {'\0'};

		strncpy(client_message, server_message, MAX_BUF_LEN );
		clock_gettime (CLOCK_REALTIME, &mt2);
		sscanf(ctime(&mt2.tv_sec), "%s %s %s %s %s", buf0, buf1, buf2, buf3, buf4);
		printf("re_buf = %s\n",client_message);
		sprintf(client_message,"Server reply to %d %s:%ld",id, buf3, mt2.tv_nsec/1000);
		char *replay_message =(char*)malloc(sizeof(client_message)+sizeof(server_message)+2);
		memset(replay_message, 0,sizeof(client_message)+sizeof(server_message)+2);
		sprintf(replay_message, "%s" "%s""\n", client_message, server_message);

		printf(" RESULT1111111   replay_message = %s\n", replay_message);

		set = write(sockfd, replay_message, MAX_BUF_LEN); ///Send the data in the send_buf buffer to the peer server
		if (set < 0)
			error_print("write");
		bzero(replay_message, strlen(replay_message));
	}
	close(sockfd);
}
//////////////////////////////////////////////////////////////////
long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}
////////////////////////////////////////////////////////////////////
int parser(char *recv_buf)
{
    char str_client[50] = {"\0"};
    char str_time[50] = {"\0"};
    int id_client = 0;
	sscanf(recv_buf,"%s %d %s",str_client, &id_client, str_time);
	//printf("+++PARSER recv_buf = %s\n",recv_buf);
	return id_client;
}
