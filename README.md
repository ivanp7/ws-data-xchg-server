# A WebSocket data exchange server

## Dependencies:

* gengetopt
* libwebsockets

## Installation

```
$ make
```

## Usage help

Use --help option.

## Provided protocols

### HTTP ("http-only")

The server responds with the simple HTML page telling that it is online.

### Broadcast echo ("broadcast-echo-protocol")

The server forwards incoming messages without any change to the all connected clients
except the message authors.

### Bulletin board ("bulletin-board-protocol")

The server implements the bulletin board intercommunication model.
After connecting to the server, a client is required to register by sending
its name with the very first message to the server. A correct name is a string
that does not contain any other characters except 'A'-'Z', 'a'-'z', '0'-'9', '-' and '\_'.
Then, the communication is carried out using the following protocol.

#### Requests:

* L -- request the list of all (excluding the requester) clients' names currently connected to the server;
* P:{bytes of data} -- publish (store) client's {bytes of data} on the server;
* R:{name1},...,{nameN} -- request the stored data of the clients with names {name1}, ..., {nameN};
* S:{name1},...,{nameN}:{bytes of data} -- send {bytes of data} directly to the clients with the specified names.

#### Responses:

* O -- success;
* F -- fail by the reason of the invalid request;
* L:{name1},...,{nameN} -- list of all (excluding the requester) clients' names currently connected to the server;
* R:{name}:{bytes of data} -- requested data of the client named {name};
* S:{name}:{bytes of data} -- data directly sent by the client named {name}.

