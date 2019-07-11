#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <pthread.h> //multithreading
#include <sys/ioctl.h> //Window sizes
#include <ncurses.h> //ncurses
#include <math.h>
#include <signal.h>

#define maxSize 4000
int sockfd, portno;
struct sockaddr_in serv_addr;
struct hostent *server;

int maxX = 0;
int maxY = 0;

char input[maxSize+2] = {0};

#define totalStoredMessages 500
char *previousMessages[totalStoredMessages];
int totalMessages = 0;

int bottomMessagePosition = 0;

int titleRows = 2;
int inputRows = 2;

void closeApp() {

	//free memory allocated for messsages	
	for(int i=0; i<sizeof(previousMessages); i++) {
		if(previousMessages[i] != NULL) {	
			free(previousMessages[i]);
			previousMessages[i] = NULL;
		}
	}
	
	endwin();
	
	exit(0);
}

void addMessage(char *msg) {

	//Make sure the memory is not already allocated
	if(previousMessages[totalMessages] == NULL) { previousMessages[totalMessages] = malloc(maxSize+1); }

	//Make sure malloc worked
	if(!previousMessages[totalMessages]) {
		return;
	}

	if(totalMessages < totalStoredMessages) {
		strncpy(previousMessages[totalMessages], msg, maxSize);
		totalMessages++;
		bottomMessagePosition++;
		//if(totalMessages-topMessagePosition > maxY-titleRows-inputRows) { topMessagePosition++; }
	} else {
		for(int i=1; i<sizeof(previousMessages)-1; i++) {
			strncpy(previousMessages[i-1], previousMessages[i], maxSize);
		}
		strncpy(previousMessages[totalStoredMessages-1], msg, maxSize);
		//if(totalMessages-topMessagePosition == maxY-titleRows-inputRows) { topMessagePosition++; }
	}

}

void redrawScreen() {

	//Clear screen
	clear();

	getmaxyx(stdscr, maxY, maxX);

	resizeterm(maxY, maxX);

	//Draw messages
	int p=maxY-2;
	for(int i=bottomMessagePosition; i>0; i--) {
		if(p <= 2) { break; }
		if(previousMessages[i] != NULL) {
			p = p - 1 - floor(strlen(previousMessages[i])/maxX);
			mvprintw(p, 0, "%s\n", previousMessages[i]);
		}
	}

	//Draw Title
	mvprintw(0, maxX/2 - strlen("TALK")/2, "TALK"); //centered
	for(int i=0; i<maxX; i++) mvprintw(1, i, "-"); //Separator
	
	//Draw serparator
	for(int i=0; i<maxX; i++) mvprintw(maxY-2, i, "-"); //Separator

	//Below is the input area
	p=0;
	for(int i=0; i<strlen(input); i++) {
		if(p>=maxX) break;
		if(i == 0 && strlen(input) > maxX) {
			i=strlen(input) - maxX;
		}
		mvprintw(maxY-1, p, "%c", input[i]);
		p++;
	}

	refresh();

}

void writeServer(char *msg) {
	int n = 0;
	if((n = (write(sockfd, msg, strlen(msg)))) < 0) {
		endwin();
		printf("ERROR while writing message: %s\n", msg);
		close(sockfd);
		pthread_cancel(pthread_self());
		closeApp();	
	} else if(n == 0) {
		endwin();
		printf("Connection lost (Server shutdown or force disconnect");
		close(sockfd);
		pthread_cancel(pthread_self());
		closeApp();	
	}
}

void *readServer() {
	char buffer[maxSize];
	bzero(buffer, sizeof(buffer));
	char current[1];
	bzero(current, sizeof(current));
	int n = 0;
	while(1) {
		//Read message character by character
		for(int i=0; i<maxSize; i++) {
			if((n = read(sockfd, current, 1)) < 0) {
				endwin();	
				printf("ERROR while receiving message\n");
				close(sockfd);
				pthread_cancel(pthread_self());
				closeApp();	
			} else if (n == 0) {
				endwin();
				printf("Connection lost (Server shutdown or force disconnect)\n");
				close(sockfd);
				pthread_cancel(pthread_self());
				closeApp();
			} else {
				if(current[0] == '\n' || i == maxSize-1) {
					addMessage(buffer);
					redrawScreen();
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

	while(1) {
		while(strlen(input) < sizeof(input)-2) {
			char c = getch();
			if (c == 26) { continue; }
			else if (c == 127 || c == 8) {
				if(strlen(input) > 0) {
					input[strlen(input)-1] = 0;
					redrawScreen();
				}	
				continue;
			}
			input[strlen(input)] = c;
			redrawScreen();
			if(c == '\n') {
				break;	
			}
		}
		input[strlen(input)+1] = '\n';
		input[strlen(input)+2] = '\0';

		writeServer(input);
		bzero(input, sizeof(input));
	}

}

int main(int argc, char *argv[]) {

    if (argc < 3) {
    	fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);	
	}
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
		printf("ERROR opening socket\n");
		exit(0);	
	}

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
        printf("ERROR connecting\n");
 		exit(0);	
	}

	initscr();
	noecho();

	//Create threads for reading and writing
	pthread_t readThread;
	pthread_t writeThread;	
	
	pthread_create(&readThread, NULL, readServer, NULL);
	pthread_create(&writeThread, NULL, getInput, NULL);

	pthread_join(readThread, NULL);
	pthread_join(writeThread, NULL);

	endwin();

}
