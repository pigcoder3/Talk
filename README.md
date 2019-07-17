# TALK

Talk is a client/server based chat system. There are no dedicated servers, so you (or someone else) will have to run one.

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

### Commands

#### Help
Description: Returns all commands
usage: `/help`

#### Rename
Description: Renames you
usage: `/name [newname]`
NOTE: `newname` must be 20 characters or less (It will be truncated if not)

#### Quit
Description: Quits the application
usage: '/quit'	