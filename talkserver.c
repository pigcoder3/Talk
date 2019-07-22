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
	int roomid;

};

struct room {
	
	int id;
	char name[21];
	struct room *next;

};

#define maxSize 4000

int sockfd, new_socket, portno, n;
socklen_t clilen;
struct sockaddr_in serv_addr;

#define maxUsers 100

struct user *usersRoot;
int usersConnected = 0;
int nextUserId = 0;

#define maxRooms maxUsers

struct room *roomsRoot;
int totalRooms = 0;
int nextRoomId = 1; //id 0 is the lobby

void writeToAllInRoom(char *msg, int roomId);
void writeToAll(char *msg);
void writeToUser( struct user *user, char *msg);

void removeroom(int id);
int addroom(char *name);
struct room *findRoomByName(char *name);
struct room *findRoomById(int id);

int numUsersInRoom(int id);

void removeUser(struct user *user) {

	if(!user) { return; }

	int roomid = user->roomid;

	int freedCurrent = 0;
	int number = 0;
	struct user *previous = malloc(sizeof(previous));
	struct user *current = usersRoot;
	while(current != NULL) {
		if(current->socket == user->socket) {
			if(number != 0 && previous != NULL) {
				if(current->next != NULL) {
					previous->next = current->next;
					current = NULL;
					free(current);
					freedCurrent = 1;
					break;
				} else {
					previous->next = NULL;
					free(previous->next);
					break;
				}
			} else { //This must be the root
				if(current->next != NULL) {
					usersRoot = current->next;
					current = NULL;
					free(current);
					freedCurrent = 1;
					break;
				} else {
					usersRoot = NULL;
					free(usersRoot);
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
	usersConnected--;

	//Check to see if the room is empty. If so delete it.
	if(roomid != 0 && numUsersInRoom(roomid) == 0) { removeroom(roomid); }
}

void addUser(struct sockaddr_in cli_addr, int socket) {

	struct user *user = (struct user*)malloc(sizeof(*user));

	user->cli_addr = cli_addr;
	user->socket = socket;
	user->id = nextUserId;
	nextUserId++;

	//Adds it to the front
	user->next = usersRoot;
	usersRoot = user;
	
	memset(user->name, 0, sizeof(user->name));

	printf("Client Connected: %s, ID: %d, Socket Descriptor: %d\n", inet_ntoa(user->cli_addr.sin_addr), user->id, socket);
	writeToUser(user, "[SERVER]: Welcome! Type '/help' for help. No other users can see your chat while in the lobby (except private messages).");

	usersConnected++;

}

struct user *findUserByName(char* name) {

	struct user *current = usersRoot;

	while(current != NULL) {
		if(strncmp(current->name, name, sizeof(20)) == 0) {
			return current;
		}	
		current = current->next;
	}

	return 0;

}

struct user *findUserById(int id) {

	struct user *current = usersRoot;

	while(current != NULL) {
		if(current->id == id) {
			return current;
		}
		current = current->next;
	}

	return 0;

}

void removeroom(int id) {

	printf("Removing room: %d\n", id);

	int freedCurrent = 0;
	int number = 0;
	struct room *previous = malloc(sizeof(struct room));
	struct room *current = roomsRoot;
	while(current != NULL) {
		if(current->id == id) {
			if(number != 0 && previous != NULL) {
				if(current->next != NULL) {
					previous->next = current->next;
					current = NULL;
					free(current);
					freedCurrent = 1;
					break;
				} else {
					previous->next = NULL;
					free(previous->next);
					break;
				}
			} else { //this must be the root
				if(current->next != NULL) {
					roomsRoot = current->next;
					current = NULL;
					free(current);
					freedCurrent = 1;
					break;
				} else {
					roomsRoot = NULL;
					free(roomsRoot);
					break;
				}
			}
		}
		number++;
		previous = current;
		current = current->next;
	}
	free(previous);
	totalRooms--;
}

int addroom(char *name) {

	struct room *room = (struct room*)malloc(sizeof(*room));

	if(findRoomByName(name)) { return -1; }

	strncpy(room->name, name, sizeof(room->name)-1);
	room->name[sizeof(room->name)-1] = '\0';
	room->id = nextRoomId;
	nextRoomId++;

	//adds it to the front
	room->next = roomsRoot;
	roomsRoot = room;
	
	printf("New room created: %s, id: %d\n", name, room->id);

	totalRooms++;

	return room->id;

}

struct room *findRoomByName(char* name) {

	struct room *current = roomsRoot;

	while(current != NULL) {
		if(strcmp(name, current->name) == 0) {
			return current;
		}	
		current = current->next;
	}

	return 0;

}

struct room *findRoomById(int id) {

	struct room *current = roomsRoot;

	while(current != NULL) {
		if(current->id == id) {
			return current;
		}
		current = current->next;
	}

	return 0;

}

int numUsersInRoom(int id) {

	int amount = 0;

	//Search through all users and check for their roomid
	struct user *current = usersRoot;
	while(current != NULL) {
		if(current->roomid == id) {
			amount++;
		}
		current = current->next;
	}

	return amount;

}

void disconnectUser(struct user *user) {

	printf("Client disconnected: %s, ID: %d\n", inet_ntoa(user->cli_addr.sin_addr), user->id);
	
	struct user *current = usersRoot;

	while(current != NULL) {
		if(current->socket != user->socket) {
			char output[102];
			if(strlen(user->name) > 0) { 
				snprintf(output, sizeof(output)-2, "[SERVER]: %s(%d) disconnected", user->name, user->id);
			} else {
				snprintf(output, sizeof(output)-2, "[SERVER]: %s(%d) disconnected", inet_ntoa(user->cli_addr.sin_addr), user->id);
			}
			writeToAllInRoom(output, user->id);
			break;
		}
		current = current->next;
	}

	removeUser(user);

}

void writeToAll(char *msg) {

	char formatted[maxSize];
	memset(formatted, 0, sizeof(formatted));
	snprintf(formatted, sizeof(formatted), "%s\n", msg);

	int n = 0;
	struct user *current = usersRoot;

	while(current != NULL) {
		if((n = (write(current->socket, formatted, strlen(formatted)+2))) < 0) {
			printf("ERROR while writing to user: %s(%d)\n", inet_ntoa(current->cli_addr.sin_addr), current->id);
			removeUser(current);
		}
		current = current->next;

	}

}

void writeToAllInRoom(char *msg, int roomId) {

	char formatted[maxSize];
	memset(formatted, 0, sizeof(formatted));
	snprintf(formatted, sizeof(formatted), "%s\n", msg);
	
	int n = 0;
	struct user *current = usersRoot;


	while(current != NULL) {
		
		if(current->roomid == roomId && (n = (write(current->socket, formatted, strlen(formatted)))) < 0) {
			printf("ERROR while writing to user: %s(%d)\n", inet_ntoa(current->cli_addr.sin_addr), current->id);
			removeUser(current);
		}
		current = current->next;
	}

	free(current);

}

void writeToUser(struct user *user, char *msg) {

	char formatted[maxSize];
	memset(formatted, 0, sizeof(formatted));
	snprintf(formatted, sizeof(formatted), "%s\n", msg);
	
	int n = 0;

	if((n = (write(user->socket, formatted, strlen(formatted)))) < 0) {
		printf("ERROR while writing to user: %s\n", inet_ntoa(user->cli_addr.sin_addr));
		removeUser(user);	
	}

}

int indexof(char character, char* string, int which) {

	int whichNumber = 1;	

	int current = 0;

	while(string[current] != 0) {
		if(string[current] == character && whichNumber == which) {
			return current;
		} else if(string[current] == character) {
			whichNumber++;
		}
		current++;
	}

	return -1;

}

char *substring(char *input, int begin, int length) {

	char *output = malloc(sizeof(output));
	memset(output, 0, sizeof(&output));

	if(length > strlen(input)) {
		length = strlen(input) - begin;
	}

	int current = begin;
	int currentBegin = 0;

	while(currentBegin < length && current < strlen(input)) {
		output[currentBegin] = input[current];
		current++;
		currentBegin++;
	}

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
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;   
    serv_addr.sin_addr.s_addr = INADDR_ANY;   
    serv_addr.sin_port = htons(portno);   
         
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("ERROR while binding. Try different port?\n");
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
   		struct user *current = usersRoot;


		while(current != NULL) {

			FD_SET(current->socket, &readfds);				

			//printf("Added socket\n");

			current = current->next;

		}

		free(current);

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
		current = usersRoot;
		while(current != NULL) {
			if (FD_ISSET( current->socket, &readfds)) {   
				char buffer[maxSize];
				memset(buffer, 0, sizeof(buffer));
				char currentChar[1];
              	memset(currentChar, 0, sizeof(currentChar));
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
							char refresh[8] = "/refresh"; //refresh rooms
							char join[5] = "/join";
							char leave[6] = "/leave";
							char create[7] = "/create";
							char allusers[9] = "/allusers";
							char users[6] = "/users";
							char help[5] = "/help";
							char quit[5] = "/quit";
							char name[5] = "/name";
							char msg[4] = "/msg";
							char msgid[6] = "/msgid";

							int maxNameSize = 20;
							
							char output[maxSize+2+maxNameSize];
							memset(output, 0, sizeof(output));
							
							if(strncmp(buffer, help, sizeof(help)) == 0) {
								char *helpOutput = malloc(11*maxSize);
								strncpy(helpOutput, "[HELP] /help - Show help menu\n"
								"[HELP] /name [newname] - change your name. New name can only be 20 characters.\n"
								"[HELP] /join [roomid] - Join a room, You must use the roomid and not the name.\n"
								"[HELP] /leave - Leave the room. When no users are left in a room, it is destroyed.\n"
								"[HELP] /create [roomname] - Create a room. Two rooms cannot have the same name.\n"
								"[HELP] /msg [username] [msg] - Send a private message. Specify recipient by name. The receiver must have a name (ip address not displayed). Private messages are not logged on the server.\n"
								"[HELP] /msgid - Send a private message, but find specify recipient by id. Private messages are not logged.\n"
								"[HELP] /allusers - Get the number of users on the server.\n"
								"[HELP] /users - Get the number of users in the room.\n"
								"[HELP] /quit - Quit the app\n"
								"For anything else, read the README that came with your copy", 11*maxSize);
								writeToUser(current, helpOutput);
								free(helpOutput);
							} else if(strncmp(buffer, quit, sizeof(quit)) == 0) { //Quit
								disconnectUser(current);
							} else if(strncmp(buffer, users, sizeof(users)) == 0) { //Total users in room
								//Make sure the user is in a room
								if(current->roomid != 0) {
									writeToUser(current, "[SERVER]: You cannot use that command in the lobby.");
									break;
								}

								char message[maxSize];
								memset(message, 0, sizeof(message));
								snprintf(message, sizeof(message), "[SERVER]: There are %d users in this room.", numUsersInRoom(current->roomid));
								writeToUser(current, message);
							} else if(strncmp(buffer, allusers, sizeof(allusers)) == 0) { //Total users on server

								char message[maxSize];
								memset(message, 0, sizeof(message));
								snprintf(message, sizeof(message), "[SERVER]: There are %d users on the server.", usersConnected);
								writeToUser(current, message);

							} else if(strncmp(buffer, msg, sizeof(msg)) == 0 || strncmp(buffer, msgid, sizeof(msgid)-1) == 0) { // private message
								int idtype = 0;

								if(strncmp(buffer, msgid, sizeof(msgid)) == 0) { //Private messaging by name
									idtype = 1;	
								} else if(strncmp(buffer, msg, sizeof(msg)) == 0) { //Private message by id
									idtype = 0;	
								}
								
								//Private messages are not logged on the server
								char user[maxSize+1];
								memset(user, 0, sizeof(user));
								char message[maxSize+1];
								memset(message, 0, sizeof(message));
								int spaceindex = indexof(' ', buffer, 2);
								if(spaceindex != -1) {
									if(idtype == 0) {
										strncpy(user, substring(buffer, sizeof(msg)+1, sizeof(buffer) - sizeof(msg)), sizeof(buffer) - sizeof(msg)-1);
										strncpy(message, substring(buffer, spaceindex + 1, sizeof(buffer) - spaceindex - 1), sizeof(buffer)-spaceindex-1);
									} else if(idtype == 1) {
										strncpy(user, substring(buffer, sizeof(msgid)+1, spaceindex - sizeof(msgid)), spaceindex - sizeof(msgid)-1);
										strncpy(message, substring(buffer, spaceindex + 1, sizeof(buffer) - spaceindex - 1), sizeof(buffer)-spaceindex-1);
									}
								} else {
									if(idtype == 0){
										strncpy(user, substring(buffer, sizeof(msg)+1, sizeof(buffer) - sizeof(msg)), sizeof(buffer) - sizeof(msg)-1);
										strncpy(message, "", 0);	
									} else if(idtype == 1) {
										strncpy(user, substring(buffer, sizeof(msgid)+1, sizeof(buffer) - sizeof(msgid)), sizeof(buffer) - sizeof(msgid)-1);
										strncpy(message, "", 0);	
									}
								}
								struct user *receiver;

								if(idtype == 0) { //Private messaging by name
									receiver = findUserByName(user);
								} else if(idtype == 1) { //Private message by id
									receiver = findUserById(atoi(user));	
								}
								
								//If a user does not exist
								if(!receiver) { 
									char noUser[100+maxNameSize];
									memset(noUser, 0, sizeof(noUser));
									if(idtype == 0) {
										snprintf(noUser, sizeof(noUser), "[SERVER]: The user '%s' does not exist!\n", user);
									} else if(idtype == 1) {
										snprintf(noUser, sizeof(noUser), "[SERVER]: The user with id, '%s', does not exist!\n", user);
									}
									writeToUser(current, noUser); 
									break; 
								}

								//If the user tries to message theirself
								if(receiver->id == current->id) { 
									writeToUser(current, "[SERVER]: You are attempting to message yourself");
									break; 
								}
	
								char formattedMessage[maxSize+1];
								//Build the message	
								message[strlen(message)] = '\0';
								//Format the message, make sure to put the ip or the name when necessary
								if(strlen(current->name) > 0) {
									if(strlen(receiver->name) > 0) {
										snprintf(formattedMessage, maxSize, "[PRIVATE] [%s(%d) -> %s(%d)]: %s", current->name, current->id, receiver->name, receiver->id, message);
									} else {
										snprintf(formattedMessage, maxSize, "[PRIVATE] [%s(%d) -> %s(%d)]: %s", current->name, current->id, inet_ntoa(receiver->cli_addr.sin_addr), receiver->id, message);
									}
								} else {
									if(strlen(receiver->name) > 0) {
										snprintf(formattedMessage, maxSize, "[PRIVATE] [%s(%d) -> %s(%d)]: %s", inet_ntoa(current->cli_addr.sin_addr), current->id, receiver->name, receiver->id, message);
									} else {
										snprintf(formattedMessage, maxSize, "[PRIVATE] [%s(%d) -> %s(%d)]: %s", inet_ntoa(current->cli_addr.sin_addr), current->id, inet_ntoa(receiver->cli_addr.sin_addr), receiver->id, message);
									}
								}
								//Send the message
								writeToUser(receiver, formattedMessage);
								writeToUser(current, formattedMessage);
								free(receiver);
							} else if(strncmp(buffer, name, sizeof(name)) == 0) { //name change
								char oldname[maxNameSize];
								memset(oldname, 0, sizeof(oldname));
								if(strlen(current->name) > 0) { strcpy(oldname, current->name); }
								
								//Reseting the output variable
								memset(output, 0, sizeof(output));

								//Getting the location of the second space in order to get the name requested
								int spaceindex = indexof(' ', buffer, 2);
								if(spaceindex != -1) {
									strncpy(output, substring(buffer, sizeof(name)+1, spaceindex), spaceindex);
								} else {
									strncpy(output, substring(buffer, sizeof(name)+1, sizeof(buffer) - sizeof(name)), sizeof(buffer) - sizeof(name));
								}
								output[strlen(output)] = '\0';
								
								//If the name has been taken
								if(findUserByName(output)) { 
									char nameUsed[100+maxNameSize];
									memset(nameUsed, 0, sizeof(nameUsed));
									snprintf(nameUsed, sizeof(nameUsed), "[SERVER]: The name '%s' is already taken!", output);
									writeToUser(current, nameUsed);
									break; 
								}
								strncpy(current->name, output, maxNameSize);
								char message[maxSize];
								//Put oldname or ip when necessary
								if(strlen(oldname) > 0) { 
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) is now known as \"%s(%d)\"", oldname, current->id, current->name, current->id);
								} else {
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) is now known as \"%s(%d)\"", inet_ntoa(current->cli_addr.sin_addr), current->id, current->name, current->id);
								}
								writeToAll(message);
								printf("%s\n", message);
							} else if(strncmp(buffer, refresh, sizeof(refresh)) == 0) { //Give rooms please
								//begin with the opening rooms tag
								char roomTag[8] = "<rooms>";
								snprintf(output, sizeof(output), "%s", roomTag);	
								writeToUser(current, output);

								struct room *currentRoom = roomsRoot;
								while(currentRoom != NULL) {	
									char roomString[maxSize];
									memset(roomString, 0, sizeof(roomString));
									snprintf(roomString, sizeof(roomString), "[ROOM] \"%s\", id:%d, users:%d", currentRoom->name, currentRoom->id, numUsersInRoom(currentRoom->id));
									writeToUser(current, roomString);
									
									currentRoom = currentRoom->next;	
								}

								//End with the closing rooms tag
								writeToUser(current, output);
								memset(output, 0, sizeof(output));
							} else if(strncmp(buffer, join, sizeof(join)) == 0) { //Join room

								//Make sure the user in not in a room
								if(current->roomid != 0) {
									writeToUser(current, "[SERVER]: You cannot use that command while in a room.");
									break;
								}

								char roomId[maxNameSize];
								memset(roomId, 0, sizeof(roomId));
								strncpy(roomId, substring(buffer, sizeof(join)+1, sizeof(buffer) - sizeof(join)), sizeof(roomId));
								struct room *room = findRoomById(atoi(roomId));

								if(!room) { 
									char message[maxSize];
									memset(message, 0, sizeof(message));
									snprintf(message, sizeof(message), "[SERVER]: The room with id '%d' does not exist!", atoi(roomId));
									writeToUser(current, message);
									break;
								} 
						
								current->roomid = atoi(roomId);
								
								//Indicate to the client that they have joined a room
								char message[maxSize];
								memset(message, 0, sizeof(message));
								writeToUser(current, "<joined>");
								snprintf(message, sizeof(message), "%s(%d)", findRoomById(current->roomid)->name, current->roomid);
								writeToUser(current, message);
								writeToUser(current, "<joined>");

snprintf(message, sizeof(message), "[SERVER]: %s(%d) joined the room", current->name, current->id);

								//Indicate to the user that they have joined a room
								writeToUser(current, "[SERVER]: You joined a room.");

								//Tell all users in that room that the user has left
								memset(message, 0, sizeof(message));
								if(strlen(current->name) > 0) { 
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) joined the room", current->name, current->id);
								} else {
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) joined the room", inet_ntoa(current->cli_addr.sin_addr), current->id);
								}
								writeToAllInRoom(message, current->roomid);

							} else if(strncmp(buffer, leave, sizeof(leave)) == 0) { //Leave room

								//Make sure the user in not in the lobby
								if(current->roomid == 0) {
									writeToUser(current, "[SERVER]: You cannot use that command while in the lobby.");
									break;
								}

								//Indicate to the client that they have left the room
								writeToUser(current, "<left>");

								//indicate to the user that they have left the room
								writeToUser(current, "[SERVER]: You left the room.");

								int lastRoomId = current->roomid;

								current->roomid = 0;

								//tell all users in that room that the user has left
								char message[maxSize];
								memset(message, 0, sizeof(message));
								if(strlen(current->name) > 0) { 
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) left the room", current->name, lastRoomId);
								} else {
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) left the room", inet_ntoa(current->cli_addr.sin_addr), lastRoomId);
								}
								writeToAllInRoom(message, lastRoomId);

								//Check to see if the room is empty. If so delete it.
								if(numUsersInRoom(lastRoomId) < 1) { removeroom(lastRoomId); }

							} else if(strncmp(buffer, create, sizeof(create)) == 0) { //Create room
								//Make sure the user in not in a room
								if(current->roomid != 0) {
									writeToUser(current, "[SERVER]: You cannot use that command while in a room.");
									break;
								}
								
								char roomName[maxNameSize];
								memset(roomName, 0, sizeof(roomName));
								strncpy(roomName, substring(buffer, sizeof(create)+1, sizeof(buffer) - sizeof(create)), sizeof(roomName));
								struct room *room = findRoomByName(roomName);

								if(room) { 
									char message[maxSize];
									memset(message, 0, sizeof(message));
									snprintf(message, sizeof(message), "[SERVER]: There is already a room with the name '%s'!", roomName);
									writeToUser(current, message);
									break;
								} 

								//Put the user in the room that they created
								current->roomid = addroom(roomName);
							
								//Indicate to the client that they have joined a room
								char message[maxSize];
								memset(message, 0, sizeof(message));
								writeToUser(current, "<joined>");
								snprintf(message, sizeof(message), "%s(%d)", findRoomById(current->roomid)->name, current->roomid);
								writeToUser(current, message);
								writeToUser(current, "<joined>");

								//Indicate to the user that they have joined a room
								writeToUser(current, "[SERVER]: You joined a room.");
						
								//Tell all users in that room that the user has joined
								memset(message, 0, sizeof(message));
								if(strlen(current->name) > 0) { 
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) joined the room", current->name, current->id);
								} else {
									snprintf(message, sizeof(message), "[SERVER]: %s(%d) joined the room", inet_ntoa(current->cli_addr.sin_addr), current->id);
								}
								writeToAllInRoom(message, current->roomid);
							} else { // no commands
								//Put name or ip when necessary
								memset(output, 0, sizeof(output));
								if(strlen(current->name) > 0) { 	
									snprintf(output, sizeof(output), "[%s(%d)]: %s", current->name, current->id, buffer);
								} else {
									snprintf(output, sizeof(output), "[%s(%d)]: %s", inet_ntoa(current->cli_addr.sin_addr), current->id, buffer);
								}
								printf("%s\n", output);
								if(current->roomid == 0) {
									writeToUser(current, output);
								} else {
									writeToAllInRoom(output, current->roomid);
								}
								memset(output, 0, sizeof(output));
							}
							break;
						} else {
							buffer[i] = currentChar[0];
						}
					}
				}
				memset(buffer, 0, sizeof(buffer));
				memset(currentChar, 0, sizeof(currentChar));
				break;
            } else {
				current = current->next;
			}
		}
	}
}
