#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h>

int maxSize = 4000;
int sockfd, portno;
struct sockaddr_in serv_addr;
struct hostent *server;

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void writeServer(char *msg) {
	//printf("Wrote: %s\n", msg);
	int n = 0;
	if((n = (write(sockfd, msg, strlen(msg)))) < 0) {
		printf("ERROR while writing message: %s\n", msg);
		close(sockfd);
		pthread_cancel(pthread_self());
		exit(0);
	} else if(n == 0) {
		printf("Connection lost (Server shutdown or force disconnect");
		close(sockfd);
		pthread_cancel(pthread_self());
		exit(0);
	}
}

void *readServer() {
	char buffer[maxSize];
	bzero(buffer, sizeof(buffer));
	char current[1];
	int n = 0;
	while(1) {
		//Read message character by character
		for(int i=0; i<maxSize; i++) {
			if((n = read(sockfd, current, 1)) < 0) {
				printf("ERROR while receiving message\n");
				close(sockfd);
				pthread_cancel(pthread_self());
				exit(0);
			} else if (n == 0) {
				printf("Connection lost (Server shutdown or force disconnect)\n");
				close(sockfd);
				pthread_cancel(pthread_self());
				exit(0);
			} else {
				if(current[0] == '\n' || i == maxSize-1) {
					printf("%s\n", buffer);
					break;
				} else {
					buffer[i] = current[0];
				}
				bzero(current, sizeof(current));
			}
		}
		bzero(buffer, sizeof(buffer));
		bzero(current, sizeof(current));
	}

}

void *getInput() {

	char buffer[maxSize+2];
	while(1) {
		fgets(buffer, maxSize, stdin);
		buffer[strlen(buffer)+1] = '\n';
		buffer[strlen(buffer)+2] = '\0';

		writeServer(buffer);
		bzero(buffer, sizeof(buffer));
		//printf("In buffer: %s\n", buffer);
	}

}

int main(int argc, char *argv[]) {

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket\n");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        error("ERROR connecting\n");
 		exit(0);	
	}

	//Create threads for reading and writing
	pthread_t readThread;
	pthread_t writeThread;	
	
	pthread_create(&readThread, NULL, readServer, NULL);
	pthread_create(&writeThread, NULL, getInput, NULL);

	pthread_join(readThread, NULL);
	pthread_join(writeThread, NULL);

}
