# TALK

Talk is a client/server text based chat system. There are no dedicated servers, so you (or someone else) will have to run one.

## Server

### Running

```
make server
./server [port]
```

## Client

### Running

```
make client
./client [serveraddress] [port]
```

### Using

Use the `/help` command for help.

#### Modes

There are two modes in Talk: INSERT mode and VIEW mode. Current mode is indicated in the bottom right corner.

INSERT mode allows you to edit and send a message.

VIEW mode allows you to scroll through your current message and the message history.

To scroll the current message:
`h`: left
`l`: right

To scroll the message history (If you are in the lobby, then you will scroll through the rooms instead):
`j`: down
`k`: up

VIEW mode allows you to edit beginning parts of the message without deleting everything after it.

#### Rooms

When users first join, they are put into the lobby. In the lobby, users cannot communicate normally (Only through private messaging). 

Instead there are two areas: one for rooms, and one for messages. The rooms area shows the all rooms on the server. In order to refresh them, run the `/refresh` command.

To join a room, run the `/join [roomid]` command. To create a new one: `/create [roomname]`.  

When a user is in a room, messages will be seen by all users. 

To leave a room, run the `/leave` command. If you were the last person in the room, it will be destroyed.

### Commands

#### Help
Description: Returns all commands

usage: `/help`

#### Rename
Description: Renames you

usage: `/name [newname]`

NOTE: `newname` must be 20 characters or less (It will be truncated if not)

#### Join
Description: joins a room

usage: `/join [roomid]`

NOTE: You must use the room id, not the name

#### Create
Description: creates a room

usage: `/create [roomname]`

NOTE: There must be no other rooms with that name.

#### Leave
Description: leaves the room

usafe: `/leave`

NOTE; If you are the last one in the room when you leave, it will be destroyed

#### Private Message
Description: Sends a message to a user. There are two commands for this.

NOTE: Private messages are not logged on the server.

##### By Name
usage: `/msg [username] [message]`

NOTE: The receiver must have a name (not the ip address displayed).

##### By Id
usage: `/msgid [userid] [message]`

NOTE: Do not use the user's name, but his/her id

#### Users

##### On Server
usage: `/allusers`

##### In Room
usage: `/users`

NOTE: This cannot be used in the lobby.

#### Quit
Description: Quits the application

usage: `/quit`	
