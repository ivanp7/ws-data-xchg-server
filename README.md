# A WebSocket data exchange server

## Dependencies

* gengetopt
* libwebsockets
* Protocol Buffers

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
except the message authors.

## Bulletin board ("bulletin-board-protocol")

The server implements the bulletin board intercommunication model.
See the [bulletin board protocol buffers file](protocols/bulletin-board/protobuf/protocol.proto).

### Operations

If got an invalid or malformed request, the server responds with a message with the following info:
```
response_type = AUX_INFO
aux_info[0].str = "error"
aux_info[0].num = <error code>
```

#### Registration

Right after connecting to the server, a client is required to register by sending
a message with the following info:
```
request_type = REGISTER
aux_info[0].str = <client name>
```

A correct name is a string shorter than 16 characters
that does not contain any other characters except 'A'-'Z', 'a'-'z', '0'-'9', '-' and '\_'.

#### Querying meta info

Querying is carried out with the following request:
```
request_type = QUERY
aux_info[0].str = <wanted info type>
```

Supported info types at the moment include:
* "client-name-list" -- get a list of names of all connected registered clients, excluding the self;
* "data-field-list" -- get a list of field names of the specified clients.

The server will respond with the 'META\_INFO' type of message, which will
contain the requested info in the corresponding places.

#### Downloading data

Downloading is carried out with the following request:
```
request_type = DOWNLOAD
clients = <wanted client names list>
data[].name = <wanted data fields>
```

The server will respond with the bunch of messages of the following format:
```
response_type = DATA
aux_info[0].str = "data-requested"
clients = <name of the data owner>
data = <requested data>
```

#### Uploading data

Uploading is carried out with the following request:
```
request_type = UPLOAD
data = <uploaded data>
```

The server will respond with the confirmation:
```
response_type = AUX_INFO
aux_info[0].str = "error"
aux_info[0].num = 0
```

#### Forwarding data

Forwarding is carried out with the following request:
```
request_type = FORWARD
clients = <recipient client names list>
data = <sent data>
```

The recipients will receive the following messages:
```
response_type = DATA
aux_info[0].str = "data-sent"
clients = <name of the data sender>
data = <sent data>
```

