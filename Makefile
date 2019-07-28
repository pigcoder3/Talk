CLIENTFLAGS= -o client -lncurses -pthread -lm
SERVERFLAGS= -o server -pthread

all: talkclient.c talkserver.c
	gcc talkclient.c $(CLIENTFLAGS) && gcc talkserver.c $(SERVERFLAGS)

client: talkclient.c
	gcc talkclient.c $(CLIENTFLAGS)
server: talkserver.c
	gcc talkserver.c $(SERVERFLAGS)
