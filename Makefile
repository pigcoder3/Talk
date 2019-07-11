CLIENTFLAGS= -o client -lncurses -pthread
SERVERFLAGS= -o server

all: talkclient.c talkserver.c
	gcc $(CLIENTFLAGS) talkclient.c && gcc $(SERVERFLAGS) talkserver.c

client: talkclient.c
	gcc $(CLIENTFLAGS) talkclient.c
server: talkserver.c
	gcc $(SERVERFLAGS) talkserver.c 
