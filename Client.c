#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "myfunction.h"
#include "printerror.h"
#include "DBGpthread.h"

#define MAX_BUF_SIZE 1024 // Maximum size of UDP messages
#define BOOLEAN int
#define FALSE 0
#define TRUE 1

typedef struct _Arg {
	int sfd;
	struct sockaddr_in server_addr;
} Arg;

void *captureServerMessageAndPrint(void *arg);
void *captureICMPPacket(void *arg);

int main(int argc, char *argv[]) {
	pthread_t tid;
	struct sockaddr_in server_addr; // struct containing server address information
	struct sockaddr_in client_addr; // struct containing client address information
	int sfd; // Server socket filed descriptor
	int rc;
	ssize_t byteSent; // Number of bytes to be sent
	size_t msgLen;
	char sendData [MAX_BUF_SIZE]; // Data to be sent
	Arg *p;

	sfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sfd < 0) {
		perror("socket UDP"); // Print error message
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[2]));
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);

	//Initialize struct with parameters to be passed to the thread
	p = malloc(sizeof(Arg));
	if (p == NULL) {
		printf("Fail malloc!");
		fflush(stdout);
		return 0;
	}
	p->sfd = sfd;
	p->server_addr = server_addr;

	//Create thread for capture messages by the user and send the messages to the server
	rc = pthread_create(&tid, NULL, captureServerMessageAndPrint, (void*)p);
	if (rc) PrintERROR_andExit(rc,"pthread_create failed");
	
	sendData[0] = '\0';

	while (TRUE) {
		fgets(sendData, MAX_BUF_SIZE, stdin);
		if (sendData[0] != '\0') {
			strtok(sendData, "\n"); //remove '\n' character by sendData
			printf("String going to be sent to server: %s\n", sendData);
			fflush(stdout);
			msgLen = countStrLen(sendData);
			byteSent = sendto(sfd, sendData, msgLen, 0, (struct sockaddr *) &server_addr, sizeof(server_addr));
			printf("Bytes sent to server: %zd\n", byteSent);
			fflush(stdout);
			sendData[0] = '\0';
		}
	}
	return 0;
}

//Function for thread
void *captureServerMessageAndPrint(void *arg) {
	char receivedData[MAX_BUF_SIZE]; // Data to be received
	ssize_t byteRecv; // Number of bytes received
	BOOLEAN stop = FALSE;
	int sfd = ((Arg*)arg)->sfd;
	struct sockaddr_in server_addr = ((Arg*)arg)->server_addr;
	socklen_t serv_size = sizeof(server_addr);;
	
	free(arg);

	while (!stop) {
		byteRecv = recvfrom(sfd, receivedData, MAX_BUF_SIZE, 0, (struct sockaddr *) &server_addr, &serv_size);
		if(byteRecv < MAX_BUF_SIZE) {
			receivedData[byteRecv] = '\0';
		}
		printf("Received from server: ");
		fflush(stdout);
		if (strcmp(receivedData, "goodbye") == 0) {
			stop = TRUE;
		}
		printData(receivedData, byteRecv);
	}
	exit(0);
}
