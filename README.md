# fty-rest

## Protocols

### STREAM DELIVER

#### SSE generic message

The SSE servlet consumes generic messages on the STREAM-DELIVER stream.

The message subject *must* be "SSE".

The message shall be frame formated as:

TOPIC/JSON\_PAYLOAD[/ASSET\_INAME]

where:

* TOPIC is a topic identifier
* JSON\_PAYLOAD is a JSON well-formed object
* ASSET\_INAME is an asset internal name (optional frame)
