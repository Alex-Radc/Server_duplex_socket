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

void create_server(void *a);
int initial_receive(int *conn,int *sockfd) ;
int initial_send(int *conn, int *sockfd, int client_id);
void send_to_client(int id, int sockfd);
int receive_from_client(int conn, int sockfd);
int parser(char *recv_buf);

int temp = 0;
//////////////////////////////////////////////////////////////////
int main(void)
{
	int sockfd = 0;
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
	printf("-->init_receive\n");
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
	//int numb_client = 0;
	//printf("initial_receive() \n");
	//printf("Timestamp: %d\n",(int)time(NULL));
	if (listen(*sockfd, SOMAXCONN) < 0)
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
	long int id = 0;
	printf("-->receive_from_client()\n");
	//printf("Timestamp: %d\n",(int)time(NULL));
	char recv_buf[MAX_BUF_LEN] = { 0 };
	while (1)
	{
		int ret = read(conn, recv_buf, MAX_BUF_LEN);//Read data sent by conn connection
		printf("ret = %d\n",ret);
		if (ret < 0)
			error_print("read");
		else if (ret == 0)
		{
			printf("client is close!\n");
			break; //The parent process receives the server-side exit information (the server Ctrl+C ends the communication process, the read function returns a value of 0, and the loop exits)
		}
		//fputs(recv_buf, stdout);
		printf("resv_buf = %s\n",recv_buf);

		// создать новый поток и потом передать его функции send_to_client();
		id = parser(recv_buf);
		{
			pthread_t thread;
			int status = 0;

			status = pthread_create(&thread, NULL, (void *)create_server, (void *) id); // change 13 to client id
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
void create_server(void *id)
{
	printf("create_server()\n");
	int sockfd = 0;
	int conn = 0;
	int client_id = (long int)id;
	// send information
	initial_send(&conn, &sockfd, client_id);
	send_to_client(client_id, sockfd);
	temp++;
	printf("temp = %d\n", temp);
}
///////////////////////////////////////////////////////////////////
int initial_send(int *conn, int *sockfd, int client_id)
{

	struct sockaddr_in servaddr;
	//printf("Timestamp: %d\n",(int)time(NULL));


	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd < 0)
		error_print("socket");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = MYPORT + client_id;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//printf("PORT from server() = %d\n", servaddr.sin_port);
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
void send_to_client(int id, int sockfd)
{
	//int conn = 0;
	struct timespec mt1;
	int set = -1;
	char recv_buf[MAX_BUF_LEN] = {0};
	//printf("send_to_client() Hello its me \n");
	//printf("Timestamp: %d\n",(int)time(NULL));
	if (3 > sockfd)
	{
		printf("invalid input param\n");
		return;
	}
	/*Used in the child process to send data to the client*/
	//signal(SIGUSR1, quit_tranmission);/*Callback function to handle communication interruption*/
	{
		char buf0[20] = {'\0'};
		char buf1[20] = {'\0'};
		char buf2[20] = {'\0'};
		char buf3[20] = {'\0'};
		char buf4[20] = {'\0'};

		clock_gettime (CLOCK_REALTIME, &mt1);
		sscanf(ctime(&mt1.tv_sec), "%s %s %s %s %s", buf0, buf1, buf2, buf3, buf4);
		sprintf(recv_buf,"Server %d %s:%ld\n",id, buf3, mt1.tv_nsec/1000);
		printf("sockfd = %d\n", sockfd);
		set = write(sockfd, recv_buf, strlen(recv_buf)); ///Send the data in the send_buf buffer to the peer server
		if (set < 0)
			error_print("write");
		bzero(recv_buf, strlen(recv_buf));
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
	//printf("<Numb client> = %d\n",id_client);
	return id_client;
}
