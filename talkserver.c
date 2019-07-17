#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

struct user {

	struct sockaddr_in cli_addr;
	int id;
	int socket;
	struct user *next;
	char name[21];

};

#define maxSize 4000

int sockfd, new_socket, portno, n;
socklen_t clilen;
struct sockaddr_in serv_addr;

#define maxUsers 100

struct user *root;
int usersConnected = 0;
int nextId = 0;

void writeToAll(char *msg);
void writeToUser( struct user *user, char *msg);

void removeUser(struct user *user) {

	if(!user) { return; }

	int number = 0;
	struct user *previous = malloc(sizeof(user));
	if(previous == NULL) { printf("Out of memory."); }
	struct user *current = root;
	while(current != NULL) {
		if(current->socket == user->socket) {
			if(number != 0 && previous != NULL) {
				if(current->next != NULL) {
					previous->next = current->next;
					current = NULL;
					break;
				} else {
					previous->next = NULL;
					break;
				}
			} else { //This must be the root
				if(current->next != NULL) {
					root = current->next;
					current = NULL;
					break;
				} else {
					root = NULL;
					break;
				}
			}
		}
		number++;
		previous = current;
		current = current->next;
	}
	free(previous);
	close(user->socket);
	usersConnected = usersConnected - 1;
}

void addUser(struct sockaddr_in cli_addr, int socket) {

	struct user *user = (struct user*)malloc(sizeof(*user));

	user->cli_addr = cli_addr;
	user->socket = socket;
	user->id = nextId;
	nextId++;

	//Adds it to the front
	user->next = root;
	root = user;
	
	bzero(user->name, sizeof(user->name));

	char output[102];
	snprintf(output, sizeof(output)-2,"[SERVER]: %s(%d) joined the chat!", inet_ntoa(user->cli_addr.sin_addr), user->id);
	writeToAll(output);

	char welcome[100] = "[SERVER]: Welcome! Type '/help' for help.";
	printf("Client Connected: %s, ID: %d, Socket Descriptor: %d\n", inet_ntoa(user->cli_addr.sin_addr), user->id, socket);
	writeToUser(user, welcome);

	usersConnected = usersConnected + 1;

}

void disconnectUser(struct user *user) {

	printf("Client disconnected: %s, ID: %d\n", inet_ntoa(user->cli_addr.sin_addr), user->id);
	
	struct user *current = root;

	while(current != NULL) {
		if(current->socket != user->socket) {
			char output[102];
			if(strlen(user->name) > 0) { 
				snprintf(output, sizeof(output)-2, "[SERVER]: %s(%d) left the chat", user->name, user->id);
			} else {
				snprintf(output, sizeof(output)-2, "[SERVER]: %s(%d) left the chat", inet_ntoa(user->cli_addr.sin_addr), user->id);
			}
			writeToAll(output);
			break;
		}
		current = current->next;
	}

	removeUser(user);

}

void writeToAll(char *msg) {

	msg[strlen(msg)+1] = '\n';
	msg[strlen(msg)+2] = '\0';

	int n = 0;
	struct user *current = root;

	while(current != NULL) {
		if((n = (write(current->socket, msg, strlen(msg)+2))) < 0) {
			printf("ERROR while writing to user: %s(%d)\n", inet_ntoa(current->cli_addr.sin_addr), current->id);
			removeUser(current);
		}
		current = current->next;

	}

}

void writeToUser(struct user *user, char *msg) {

	msg[strlen(msg)+1] = '\n';
	msg[strlen(msg)+2] = '\0';
	int n = 0;
	
	if((n = (write(user->socket, msg, strlen(msg)+2))) < 0) {
		printf("ERROR while writing to user: %s\n", inet_ntoa(user->cli_addr.sin_addr));
		removeUser(user);	
	}

}

char *substring(char *input, int begin, int length) {

	printf("%s", input);

	char *output = malloc(sizeof(output));
	bzero(output, sizeof(&output));

	if(length > strlen(input)) {
		length = strlen(input) - begin;
	}

	int current = begin;
	int currentBegin = 0;

	while(currentBegin < length && current < strlen(input)) {
		printf("%c", input[current]);
		output[currentBegin] = input[current];
		current++;
		currentBegin++;
	}

	printf("%d", currentBegin);

	output[currentBegin] = '\0';

	return output;

}

int main(int argc, char *argv[]){
	
	//Check to make sure the port was given	
	if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
   	}

	//Open the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
		printf("ERROR opening socket\n");

	//printf("%d", sockfd);

	//Bind the port	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;   
    serv_addr.sin_addr.s_addr = INADDR_ANY;   
    serv_addr.sin_port = htons(portno);   
         
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR while binding\n");
		exit(0);
	}
         
    if (listen(sockfd, 5) < 0) {   
        printf("ERROR while listening for connections\n");   
   		exit(0); 
	} 
		

	printf("Server up and running\n");

	while(1) {   

		fd_set readfds;

        //clear the socket set  
        FD_ZERO(&readfds);   
     
        //add master socket to set  
        FD_SET(sockfd, &readfds);   
        int max_sd = sockfd;
   		struct user *current = root;


		while(current != NULL) {

			FD_SET(current->socket, &readfds);				

			//printf("Added socket\n");

			current = current->next;

		}

        //wait for an activity on one of the sockets , timeout is NULL ,  
        //so wait indefinitely  
		//printf("Waiting for activity\n");
        if ((select( max_sd + usersConnected + 1 , &readfds , NULL , NULL , NULL)) < 0) {   
            printf("ERROR when waiting for messages\n");   
        }   
           
		struct sockaddr_in cli_addr;
		int clilen = sizeof(cli_addr);
        //If something happened on the master socket ,  
        //then its an incoming connection  
        if (FD_ISSET(sockfd, &readfds)) {   
            if ((new_socket = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen))<0) { 
                printf("ERROR when accepting connection\n");   
            }   
            //send new connection greeting message  
			if(usersConnected < 100) {
				//Add new user
            	addUser(cli_addr, new_socket);
			} else {
				if( send(new_socket, "Server full", strlen("Server full"), 0) != strlen("Server full") ) {   
               		printf("ERROR when sending full message\n");   
					close(new_socket);
            	}
				close(new_socket);
			}
			//printf("Found connection\n");
		}   
             
        //else its some IO operation on some other socket 
		current = root;
		while(current != NULL) {
			if (FD_ISSET( current->socket, &readfds)) {   
				char buffer[maxSize];
				bzero(buffer, sizeof(buffer));
				char currentChar[1];
              	for(int i=0; i<sizeof(buffer)-1; i++) {
					//read message
					if((n = read(current->socket, currentChar, 1)) == -1) {
						printf("ERROR while recieving message\n");
						disconnectUser(current);	
					} else if(n == 0) {	//User disconnected
						disconnectUser(current);
						break;
					} else {
						if(currentChar[0] == '\n' || i == maxSize -1) {
							//Commands
							char help[5] = "/help";
							char quit[5] = "/quit";
							char name[5] = "/name";

							int maxNameSize = 20;
							
							char output[maxSize+2+20];
							
							if(strncmp(buffer, help, sizeof(help)-1) == 0) {
								char helpOutput[maxSize] = "[HELP] /help - Show help menu\n[HELP] /name [newname] - change your name. New name can only be 20 characters.\n[HELP] /quit - Quit the app";
								writeToUser(current, helpOutput);
							} else if(strncmp(buffer, quit, sizeof(quit)-1) == 0) { //Quit
								disconnectUser(current);
							} else if(strncmp(buffer, name, sizeof(name)-1) == 0) { //name change
								char oldname[maxNameSize];
								bzero(oldname, sizeof(oldname));
								if(strlen(current->name) > 0) { strcpy(oldname, current->name); }
								
								char output[maxNameSize+1];
								bzero(output, sizeof(output));
								strncpy(output, substring(buffer, sizeof(name)+1, maxNameSize), maxNameSize);
								output[strlen(output)] = '\0';
								strncpy(current->name, output, maxNameSize);
								
								char message[maxSize];
								if(strlen(oldname) > 0) { 
									snprintf(message, sizeof(message), "[SERVER] %s(%d) is now known as \"%s(%d)\"", oldname, current->id, current->name, current->id);
								} else {
									snprintf(message, sizeof(message), "[SERVER] %s(%d) is now known as \"%s(%d)\"", inet_ntoa(current->cli_addr.sin_addr), current->id, current->name, current->id);
								}
								
								writeToAll(message);
								printf("%s\n", message);
							} else { // no commands
								if(strlen(current->name) > 0) { 	
									snprintf(output, sizeof(output)-1, "[%s(%d)]: %s", current->name, current->id, buffer);
								} else {
									snprintf(output, sizeof(output)-1, "[%s(%d)]: %s", inet_ntoa(current->cli_addr.sin_addr), current->id, buffer);
								}
								printf("%s\n", output);
								writeToAll(output);
								bzero(output, sizeof(output));
							}
							break;
						} else {
							buffer[i] = currentChar[0];
						}
					}
				}
				bzero(currentChar, sizeof(currentChar));
				bzero(buffer, sizeof(buffer));
				break;
            } else {
				current = current->next;
			}
		}
	}
}
