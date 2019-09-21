#ifndef _THREAD_SAFE
	#define _THREAD_SAFE
#endif
#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#else
#if _POSIX_C_SOURCE < 200112L
	#undef  _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif
#endif
#ifndef _THREAD_SAFE
	#define _THREAD_SAFE
#endif
#ifndef _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#else
#if _POSIX_C_SOURCE < 200112L
	#undef  _POSIX_C_SOURCE
	#define _POSIX_C_SOURCE 200112L
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include "myfunction.h"
#include "printerror.h"
#include "DBGpthread.h"
#include "Queue.h"

#define MAX_BUF_SIZE 1024 // Maximum size of UDP messages
#define IPV4_LENGHT 13 // IpV4 Lenght
#define BOOLEAN int
#define FALSE 0
#define TRUE 1

typedef struct _Arg {
	int sfd;
	Queue *queueMessages;
	struct sockaddr_in *client_addr;
} Arg;

void *captureUserMessageAndSend(void *arg);
void sendMessage(int sfd, char* sendData, size_t msgLen, struct sockaddr_in client_addr);

pthread_mutex_t mutexEstablishedConnection;
BOOLEAN establishedConnection;

int main(int argc, char *argv[]) {
	pthread_t tid;
	int sfd; // Server socket filed descriptor
	int rc;
	Arg *p;
	ssize_t byteRecv; // Number of bytes received
	struct sockaddr_in server_addr; // struct containing server address information
	struct sockaddr_in client_addr; // struct containing client address information (used to maintain client information with wich established a connection)
	struct sockaddr_in tmp_client_addr; // struct containing client address information (used when a new package arrives from a client)
	socklen_t cli_size;
	char receivedData[MAX_BUF_SIZE]; // Data to be received
	char clientSourceIp[IPV4_LENGHT]; //Ip of the client with which established a connection
	char tmpClientSourceIp[IPV4_LENGHT]; //Ip of the general client
	Cell *message;
	Queue queueMessages;

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd < 0) {
		perror("socket"); // Print error message
		exit(EXIT_FAILURE);
	}
	// Initialize server address information
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2])); // Convert to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(sfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("bind"); // Print error message
		exit(EXIT_FAILURE);
	}
	cli_size = sizeof(client_addr);
	queueMessages.first = NULL;
	queueMessages.last = NULL;
	establishedConnection = FALSE;

	//Initialize mutex
	rc = pthread_mutex_init( &mutexEstablishedConnection, NULL);
	if (rc) PrintERROR_andExit(rc,"pthread_mutex_init failed");
	
	//Initialize struct with parameters to be passed to the thread
	p = malloc(sizeof(Arg));
	if (p == NULL) {
		printf("Fail malloc!");
		fflush(stdout);
		return 0;
	}
	p->sfd = sfd;
	p->queueMessages = &queueMessages;
	p->client_addr = &client_addr;
	//Create thread for capture messages by the user and send the messages to the client
	rc = pthread_create(&tid, NULL, captureUserMessageAndSend, (void*)p);
	if (rc) PrintERROR_andExit(rc,"pthread_create failed");

	while (TRUE) {
		byteRecv = recvfrom(sfd, receivedData, MAX_BUF_SIZE, 0, (struct sockaddr *) &tmp_client_addr, &cli_size);
		if (byteRecv == -1) {
			perror("recvfrom");
			exit(EXIT_FAILURE);
		}
		inet_ntop(AF_INET, &(tmp_client_addr.sin_addr), tmpClientSourceIp, IPV4_LENGHT);
		if (tmpClientSourceIp == NULL) {
			perror("inet_ntop");
			exit(EXIT_FAILURE);
		}
		printf("Client source ip: %s\n", tmpClientSourceIp);
		fflush(stdout);
		printf("Client source port: %d\n", htons(tmp_client_addr.sin_port));
		fflush(stdout);
		printf("Received data: ");
		fflush(stdout);
		printData(receivedData, byteRecv);
		DBGpthread_mutex_lock(&mutexEstablishedConnection,"main");
		if (!establishedConnection) {
			//Set the client's connection with the server
			client_addr.sin_family = tmp_client_addr.sin_family;
			client_addr.sin_port = tmp_client_addr.sin_port;
			client_addr.sin_addr.s_addr = tmp_client_addr.sin_addr.s_addr;

			inet_ntop(AF_INET, &(client_addr.sin_addr), clientSourceIp, IPV4_LENGHT);
			if (clientSourceIp == NULL) {
				perror("inet_ntop");
				exit(EXIT_FAILURE);
			}
			//Send all messages that the user was written before that the connection with the client were established
			while (queueMessages.first != NULL) {
				message = pop(&queueMessages);
				sendMessage(sfd, message->message, countStrLen(message->message), client_addr);
				free(message->message);
				free(message);
			}
			establishedConnection = TRUE;
		}
		DBGpthread_mutex_unlock(&mutexEstablishedConnection,"main");
		//If the new message by the client coming from the client that is not established connection with server 
		//or the client that have established connection send the message 'exit' then send the message 'goodbye' for
		//terminate  the client
		if (strcmp(tmpClientSourceIp, clientSourceIp) != 0
			|| (strcmp(tmpClientSourceIp, clientSourceIp) == 0) && htons(tmp_client_addr.sin_port) != htons(client_addr.sin_port)) {
			sendMessage(sfd, "goodbye", countStrLen("goodbye"), tmp_client_addr);
		} else if (strncmp(receivedData, "exit", byteRecv) == 0) {
			sendMessage(sfd, "goodbye", countStrLen("goodbye"), tmp_client_addr);
			DBGpthread_mutex_lock(&mutexEstablishedConnection,"main");
			establishedConnection = FALSE;
			DBGpthread_mutex_unlock(&mutexEstablishedConnection,"main");
		}
	}
	return 0;
}

//Function for thread
void *captureUserMessageAndSend(void *arg) {
	char sendData[MAX_BUF_SIZE]; // Data to be sent
	Cell *newMessage;
	char *p;
	int sfd = ((Arg*)arg)->sfd;
	Queue *queueMessages = ((Arg*)arg)->queueMessages;
	struct sockaddr_in *client_addr = ((Arg*)arg)->client_addr;

	free(arg);
	sendData[0] = '\0';
	while (TRUE) {
		fgets(sendData, MAX_BUF_SIZE, stdin);
		if (sendData[0] != '\0') {
			strtok(sendData, "\n"); //remove '\n' character by sendData
			DBGpthread_mutex_lock(&mutexEstablishedConnection,"captureUserMessageAndSend");
			if (establishedConnection) {
				DBGpthread_mutex_unlock(&mutexEstablishedConnection,"captureUserMessageAndSend");
				sendMessage(sfd, sendData, countStrLen(sendData), *client_addr);
			} else {
				newMessage=((Cell*)malloc(sizeof(Cell)));
				if(newMessage == NULL){
					printf("Fail malloc!");
					fflush(stdout);
					exit(0);
				}
				newMessage->next = NULL;
				p = malloc(sizeof(char) * ((int)strlen(sendData)) + 1);
				if (p == NULL) {
					printf("Fail malloc!");
					fflush(stdout);
					exit(0);
				}
				strcpy(p, sendData);
				newMessage->message = p;
				push(queueMessages,newMessage);	
				DBGpthread_mutex_unlock(&mutexEstablishedConnection,"captureUserMessageAndSend");
			}
			sendData[0] = '\0';
		}
	}
	exit(0);
}

void sendMessage(int sfd, char* sendData, size_t msgLen, struct sockaddr_in client_addr) {
	ssize_t byteSent; // Number of bytes to be sent
	byteSent = sendto(sfd, sendData, msgLen, 0, (struct sockaddr *) &client_addr, sizeof(client_addr));
	printf("Bytes sent to client: %zd\n", byteSent);
	fflush(stdout);
}
