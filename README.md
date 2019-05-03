# A WebSocket data exchange server

## Dependencies

* gengetopt
* libwebsockets
* cjson

## Installation

```
$ make
```

## Usage help

See output of the "--help" option.


# Provided WebSocket subprotocols

## HTTP ("http-only")

The server responds with the simple HTML page telling that it is online.

## Broadcast echo ("broadcast-echo-protocol")

The server forwards incoming messages without any change to the all connected clients
except the addresser.

## Bulletin board ("bulletin-board-protocol")

The server implements the bulletin board intercommunication model.

After connecting to the server, a client is required to register by sending
its name with the very first message to the server. A correct name is a string
shorter than 16 characters
that does not contain any other characters except 'A'-'Z', 'a'-'z', '0'-'9', '-' and '\_'.
The server responds with "A:K" on success and "A:F" on fail.
Then, the communication is carried out using the following protocol.
The server responds with "A:F" on any invalid request.

### Requests & responses

* request:  "L" -- request the list of all other registered clients' names currently connected to the server,

  response: "L:{name1},...,{nameN}" -- list of all other registered clients' names currently connected to the server;

* request:  "P:{bytes of data}" -- publish (store) client's {bytes of data} on the server,

  response: "A:K" on success and "A:F" on fail;

* request:  "R:{name1},...,{nameN}" -- request the stored data of the clients with names {name1}, ..., {nameN};

  response: "R:{name}:{bytes of data}" -- requested data of the client named {name};

* request:  "S:{name1},...,{nameN}:{bytes of data}" -- send {bytes of data} directly to the clients with the specified names.

  response: none or "S:{name}:{bytes of data}" -- data directly sent by the client named {name}.

## JSON transmission ("json-transmission-protocol")

Clients send JSON strings to the server. To be processed correctly, JSON must contain two fields: "from" and "to".
The "to" field specifies name of the addressee client.
The "from" field specifies name of the addresser client. If this name is changed from the last time, then it is used as a new client's name.

