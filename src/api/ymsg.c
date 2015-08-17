/*  =========================================================================
    ymsg - draft

    Codec class for ymsg.

    ** WARNING *************************************************************
    THIS SOURCE FILE IS 100% GENERATED. If you edit this file, you will lose
    your changes at the next build cycle. This is great for temporary printf
    statements. DO NOT MAKE ANY CHANGES YOU WISH TO KEEP. The correct places
    for commits are:

     * The XML model used for this code generation: rozp.xml, or
     * The code generation script that built this file: zproto_codec_c_v1
    ************************************************************************
                                                                        
    Copyright (C) 2014 - 2015 Eaton                                     
                                                                        
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or   
    (at your option) any later version.                                 
                                                                        
    This program is distributed in the hope that it will be useful,     
    but WITHOUT ANY WARRANTY; without even the implied warranty of      
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
    GNU General Public License for more details.                        
                                                                        
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    ymsg - draft
@discuss
@end
*/

#include "ymsg.h"

//  Structure of our class

struct _ymsg_t {
    zframe_t *routing_id;               //  Routing_id from ROUTER, if any
    int id;                             //  ymsg message ID
    byte *needle;                       //  Read/write pointer for serialization
    byte *ceiling;                      //  Valid upper limit for read pointer
    byte version;                       //  Protocol version
    uint16_t seq;                       //  Agent specific, starting value unspecified. Each message sent increments this number by one. Reply message must send this number back encoded in field 'rep'.
    zhash_t *aux;                       //  Extra (auxiliary) headers. Keys must contain only the following characters 'a-zA-Z_-' and values can be any sequence without '\0' (NULL) character. Users can pass non-standard user-defined headers, but they must be prefixed with 'X-'. This field can be used to carry simple key-value app data as well.
    size_t aux_bytes;                   //  Size of dictionary content
    zchunk_t *request;                  //  Application specific payload. Not mandatory.
    uint16_t rep;                       //  Value must be identical to field 'seq' of message 'send' to which this reply message is being created.
    zchunk_t *response;                 //  
};

//  --------------------------------------------------------------------------
//  Network data encoding macros

//  Put a block of octets to the frame
#define PUT_OCTETS(host,size) { \
    memcpy (self->needle, (host), size); \
    self->needle += size; \
}

//  Get a block of octets from the frame
#define GET_OCTETS(host,size) { \
    if (self->needle + size > self->ceiling) \
    {\
        zsys_error ("malformed in GET_OCTETS \n");\
        goto malformed; \
    }\
    memcpy ((host), self->needle, size); \
    self->needle += size; \
}

//  Put a 1-byte number to the frame
#define PUT_NUMBER1(host) { \
    *(byte *) self->needle = (host); \
    self->needle++; \
}

//  Put a 2-byte number to the frame
#define PUT_NUMBER2(host) { \
    self->needle [0] = (byte) (((host) >> 8)  & 255); \
    self->needle [1] = (byte) (((host))       & 255); \
    self->needle += 2; \
}

//  Put a 4-byte number to the frame
#define PUT_NUMBER4(host) { \
    self->needle [0] = (byte) (((host) >> 24) & 255); \
    self->needle [1] = (byte) (((host) >> 16) & 255); \
    self->needle [2] = (byte) (((host) >> 8)  & 255); \
    self->needle [3] = (byte) (((host))       & 255); \
    self->needle += 4; \
}

//  Put a 8-byte number to the frame
#define PUT_NUMBER8(host) { \
    self->needle [0] = (byte) (((host) >> 56) & 255); \
    self->needle [1] = (byte) (((host) >> 48) & 255); \
    self->needle [2] = (byte) (((host) >> 40) & 255); \
    self->needle [3] = (byte) (((host) >> 32) & 255); \
    self->needle [4] = (byte) (((host) >> 24) & 255); \
    self->needle [5] = (byte) (((host) >> 16) & 255); \
    self->needle [6] = (byte) (((host) >> 8)  & 255); \
    self->needle [7] = (byte) (((host))       & 255); \
    self->needle += 8; \
}

//  Get a 1-byte number from the frame
#define GET_NUMBER1(host) { \
    if (self->needle + 1 > self->ceiling) \
    {\
        zsys_error ("malformed in NUMBER1 \n");\
        goto malformed; \
    }\
    (host) = *(byte *) self->needle; \
    self->needle++; \
}

//  Get a 2-byte number from the frame
#define GET_NUMBER2(host) { \
    if (self->needle + 2 > self->ceiling) \
    {\
        zsys_error ("malformed in NUMBER2 \n");\
        goto malformed; \
    }\
    (host) = ((uint16_t) (self->needle [0]) << 8) \
           +  (uint16_t) (self->needle [1]); \
    self->needle += 2; \
}

//  Get a 4-byte number from the frame
#define GET_NUMBER4(host) { \
    if (self->needle + 4 > self->ceiling) \
    {\
        zsys_error ("malformed in NUMBER4 \n");\
        goto malformed; \
    }\
    (host) = ((uint32_t) (self->needle [0]) << 24) \
           + ((uint32_t) (self->needle [1]) << 16) \
           + ((uint32_t) (self->needle [2]) << 8) \
           +  (uint32_t) (self->needle [3]); \
    self->needle += 4; \
}

//  Get a 8-byte number from the frame
#define GET_NUMBER8(host) { \
    if (self->needle + 8 > self->ceiling) \
    {\
        zsys_error ("malformed in NUMBER8 \n");\
        goto malformed; \
    }\
    (host) = ((uint64_t) (self->needle [0]) << 56) \
           + ((uint64_t) (self->needle [1]) << 48) \
           + ((uint64_t) (self->needle [2]) << 40) \
           + ((uint64_t) (self->needle [3]) << 32) \
           + ((uint64_t) (self->needle [4]) << 24) \
           + ((uint64_t) (self->needle [5]) << 16) \
           + ((uint64_t) (self->needle [6]) << 8) \
           +  (uint64_t) (self->needle [7]); \
    self->needle += 8; \
}

//  Put a string to the frame
#define PUT_STRING(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER1 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a string from the frame
#define GET_STRING(host) { \
    size_t string_size; \
    GET_NUMBER1 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
    {\
        zsys_error ("malformed in STRING \n");\
        goto malformed; \
    }\
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}

//  Put a long string to the frame
#define PUT_LONGSTR(host) { \
    size_t string_size = strlen (host); \
    PUT_NUMBER4 (string_size); \
    memcpy (self->needle, (host), string_size); \
    self->needle += string_size; \
}

//  Get a long string from the frame
#define GET_LONGSTR(host) { \
    size_t string_size; \
    GET_NUMBER4 (string_size); \
    if (self->needle + string_size > (self->ceiling)) \
    {\
        zsys_error ("malformed in LONGSTRING \n");\
        goto malformed; \
    }\
    (host) = (char *) malloc (string_size + 1); \
    memcpy ((host), self->needle, string_size); \
    (host) [string_size] = 0; \
    self->needle += string_size; \
}


//  --------------------------------------------------------------------------
//  Create a new ymsg

ymsg_t *
ymsg_new (int id)
{
    ymsg_t *self = (ymsg_t *) zmalloc (sizeof (ymsg_t));
    self->id = id;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the ymsg

void
ymsg_destroy (ymsg_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        ymsg_t *self = *self_p;

        //  Free class properties
        zframe_destroy (&self->routing_id);
        zhash_destroy (&self->aux);
        zchunk_destroy (&self->request);
        zchunk_destroy (&self->response);

        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  Parse a zmsg_t and decides whether it is ymsg. Returns
//  true if it is, false otherwise. Doesn't destroy or modify the
//  original message.
bool
is_ymsg (zmsg_t *msg)
{
    if (msg == NULL)
        return false;

    zframe_t *frame = zmsg_first (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty


    //  Get and check protocol signature
    ymsg_t *self = ymsg_new (0);
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0))
    {
        zsys_error (" problem with signature\n");
        goto fail;             //  Invalid signature
    }

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case YMSG_SEND:
        case YMSG_REPLY:
            ymsg_destroy (&self);
            return true;
        default:
        {
            zsys_error (" problem with self_id '%d'\n", self->id);
            goto fail;
        }
    }
    empty:
        zsys_error (" empty message\n");
    fail:
    malformed:
        ymsg_destroy (&self);
        return false;
}

//  --------------------------------------------------------------------------
//  Parse a ymsg from zmsg_t. Returns a new object, or NULL if
//  the message could not be parsed, or was NULL. Destroys msg and 
//  nullifies the msg reference.

ymsg_t *
ymsg_decode (zmsg_t **msg_p)
{
    assert (msg_p);
    zmsg_t *msg = *msg_p;
    if (msg == NULL)
        return NULL;
        
    ymsg_t *self = ymsg_new (0);
    //  Read and parse command in frame
    zframe_t *frame = zmsg_pop (msg);
    if (!frame) 
        goto empty;             //  Malformed or empty

    //  Get and check protocol signature
    self->needle = zframe_data (frame);
    self->ceiling = self->needle + zframe_size (frame);
    uint16_t signature;
    GET_NUMBER2 (signature);
    if (signature != (0xAAA0 | 0))
    {
        zsys_error (" problem with signature\n");
        goto empty;             //  Invalid signature
    }

    //  Get message id and parse per message type
    GET_NUMBER1 (self->id);

    switch (self->id) {
        case YMSG_SEND:
            GET_NUMBER1 (self->version);
            GET_NUMBER2 (self->seq);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->aux = zhash_new ();
                zhash_autofree (self->aux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->aux, key, value);
                    free (key);
                    free (value);
                }
            }
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                {
                    zsys_error ("malformed in YMSG response field \n");
                    goto malformed;
                }
                self->request = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        case YMSG_REPLY:
            GET_NUMBER1 (self->version);
            GET_NUMBER2 (self->seq);
            GET_NUMBER2 (self->rep);
            {
                size_t hash_size;
                GET_NUMBER4 (hash_size);
                self->aux = zhash_new ();
                zhash_autofree (self->aux);
                while (hash_size--) {
                    char *key, *value;
                    GET_STRING (key);
                    GET_LONGSTR (value);
                    zhash_insert (self->aux, key, value);
                    free (key);
                    free (value);
                }
            }
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                {
                    zsys_error ("malformed 'response' field \n");
                    goto malformed;
                }
                self->response = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            {
                size_t chunk_size;
                GET_NUMBER4 (chunk_size);
                if (self->needle + chunk_size > (self->ceiling))
                {
                    zsys_error ("malformed 'request' field \n");
                    goto malformed;
                }
                self->request = zchunk_new (self->needle, chunk_size);
                self->needle += chunk_size;
            }
            break;

        default:
        {
            zsys_error ("malformed 'id' field\n");
            goto malformed;
        }
    }
    //  Successful return
    zframe_destroy (&frame);
    zmsg_destroy (msg_p);
    return self;

    //  Error returns
    malformed:
        zsys_error ("malformed message '%d'\n", self->id);
    empty:
        zframe_destroy (&frame);
        zmsg_destroy (msg_p);
        ymsg_destroy (&self);
        return (NULL);
}


//  --------------------------------------------------------------------------
//  Encode ymsg into zmsg and destroy it. Returns a newly created
//  object or NULL if error. Use when not in control of sending the message.

zmsg_t *
ymsg_encode (ymsg_t **self_p)
{
    assert (self_p);
    assert (*self_p);
    
    ymsg_t *self = *self_p;
    zmsg_t *msg = zmsg_new ();

    size_t frame_size = 2 + 1;          //  Signature and message ID
    switch (self->id) {
        case YMSG_SEND:
            //  version is a 1-byte integer
            frame_size += 1;
            //  seq is a 2-byte integer
            frame_size += 2;
            //  aux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->aux) {
                self->aux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    self->aux_bytes += 1 + strlen ((const char *) zhash_cursor (self->aux));
                    self->aux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            frame_size += self->aux_bytes;
            //  request is a chunk with 4-byte length
            frame_size += 4;
            if (self->request)
                frame_size += zchunk_size (self->request);
            break;
            
        case YMSG_REPLY:
            //  version is a 1-byte integer
            frame_size += 1;
            //  seq is a 2-byte integer
            frame_size += 2;
            //  rep is a 2-byte integer
            frame_size += 2;
            //  aux is an array of key=value strings
            frame_size += 4;    //  Size is 4 octets
            if (self->aux) {
                self->aux_bytes = 0;
                //  Add up size of dictionary contents
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    self->aux_bytes += 1 + strlen ((const char *) zhash_cursor (self->aux));
                    self->aux_bytes += 4 + strlen (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            frame_size += self->aux_bytes;
            //  response is a chunk with 4-byte length
            frame_size += 4;
            if (self->response)
                frame_size += zchunk_size (self->response);
            //  request is a chunk with 4-byte length
            frame_size += 4;
            if (self->request)
                frame_size += zchunk_size (self->request);
            break;
            
        default:
            zsys_error ("bad message type '%d', not sent\n", self->id);
            //  No recovery, this is a fatal application error
            assert (false);
    }
    //  Now serialize message into the frame
    zframe_t *frame = zframe_new (NULL, frame_size);
    self->needle = zframe_data (frame);
    PUT_NUMBER2 (0xAAA0 | 0);
    PUT_NUMBER1 (self->id);

    switch (self->id) {
        case YMSG_SEND:
            PUT_NUMBER1 (self->version);
            PUT_NUMBER2 (self->seq);
            if (self->aux) {
                PUT_NUMBER4 (zhash_size (self->aux));
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->aux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            if (self->request) {
                PUT_NUMBER4 (zchunk_size (self->request));
                memcpy (self->needle,
                        zchunk_data (self->request),
                        zchunk_size (self->request));
                self->needle += zchunk_size (self->request);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

        case YMSG_REPLY:
            PUT_NUMBER1 (self->version);
            PUT_NUMBER2 (self->seq);
            PUT_NUMBER2 (self->rep);
            if (self->aux) {
                PUT_NUMBER4 (zhash_size (self->aux));
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    PUT_STRING ((const char *) zhash_cursor ((zhash_t *) self->aux));
                    PUT_LONGSTR (item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                PUT_NUMBER4 (0);    //  Empty dictionary
            if (self->response) {
                PUT_NUMBER4 (zchunk_size (self->response));
                memcpy (self->needle,
                        zchunk_data (self->response),
                        zchunk_size (self->response));
                self->needle += zchunk_size (self->response);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            if (self->request) {
                PUT_NUMBER4 (zchunk_size (self->request));
                memcpy (self->needle,
                        zchunk_data (self->request),
                        zchunk_size (self->request));
                self->needle += zchunk_size (self->request);
            }
            else
                PUT_NUMBER4 (0);    //  Empty chunk
            break;

    }
    //  Now send the data frame
    if (zmsg_append (msg, &frame)) {
        zmsg_destroy (&msg);
        ymsg_destroy (self_p);
        return NULL;
    }
    //  Destroy ymsg object
    ymsg_destroy (self_p);
    return msg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a ymsg from the socket. Returns new object or
//  NULL if error. Will block if there's no message waiting.

ymsg_t *
ymsg_recv (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv (input);
    if (!msg)
        return NULL;            //  Interrupted
    zmsg_print (msg);

    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    ymsg_t *ymsg = ymsg_decode (&msg);
    if (ymsg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        ymsg->routing_id = routing_id;

    return ymsg;
}


//  --------------------------------------------------------------------------
//  Receive and parse a ymsg from the socket. Returns new object,
//  or NULL either if there was no input waiting, or the recv was interrupted.

ymsg_t *
ymsg_recv_nowait (void *input)
{
    assert (input);
    zmsg_t *msg = zmsg_recv_nowait (input);
    if (!msg)
        return NULL;            //  Interrupted
    zmsg_print (msg);
    //  If message came from a router socket, first frame is routing_id
    zframe_t *routing_id = NULL;
    if (zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER) {
        routing_id = zmsg_pop (msg);
        //  If message was not valid, forget about it
        if (!routing_id || !zmsg_next (msg))
            return NULL;        //  Malformed or empty
    }
    ymsg_t *ymsg = ymsg_decode (&msg);
    if (ymsg && zsocket_type (zsock_resolve (input)) == ZMQ_ROUTER)
        ymsg->routing_id = routing_id;

    return ymsg;
}


//  --------------------------------------------------------------------------
//  Send the ymsg to the socket, and destroy it
//  Returns 0 if OK, else -1

int
ymsg_send (ymsg_t **self_p, void *output)
{
    assert (self_p);
    assert (*self_p);
    assert (output);

    //  Save routing_id if any, as encode will destroy it
    ymsg_t *self = *self_p;
    zframe_t *routing_id = self->routing_id;
    self->routing_id = NULL;

    //  Encode ymsg message to a single zmsg
    zmsg_t *msg = ymsg_encode (self_p);
    
    //  If we're sending to a ROUTER, send the routing_id first
    if (zsocket_type (zsock_resolve (output)) == ZMQ_ROUTER) {
        assert (routing_id);
        zmsg_prepend (msg, &routing_id);
    }
    else
        zframe_destroy (&routing_id);
        
    if (msg && zmsg_send (&msg, output) == 0)
        return 0;
    else
        return -1;              //  Failed to encode, or send
}


//  --------------------------------------------------------------------------
//  Send the ymsg to the output, and do not destroy it

int
ymsg_send_again (ymsg_t *self, void *output)
{
    assert (self);
    assert (output);
    self = ymsg_dup (self);
    return ymsg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Encode SEND message

zmsg_t * 
ymsg_encode_send (
    byte version,
    uint16_t seq,
    zhash_t *aux,
    zchunk_t *request)
{
    ymsg_t *self = ymsg_new (YMSG_SEND);
    ymsg_set_version (self, version);
    ymsg_set_seq (self, seq);
    zhash_t *aux_copy = zhash_dup (aux);
    ymsg_set_aux (self, &aux_copy);
    zchunk_t *request_copy = zchunk_dup (request);
    ymsg_set_request (self, &request_copy);
    return ymsg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Encode REPLY message

zmsg_t * 
ymsg_encode_reply (
    byte version,
    uint16_t seq,
    uint16_t rep,
    zhash_t *aux,
    zchunk_t *response,
    zchunk_t *request)
{
    ymsg_t *self = ymsg_new (YMSG_REPLY);
    ymsg_set_version (self, version);
    ymsg_set_seq (self, seq);
    ymsg_set_rep (self, rep);
    zhash_t *aux_copy = zhash_dup (aux);
    ymsg_set_aux (self, &aux_copy);
    zchunk_t *response_copy = zchunk_dup (response);
    ymsg_set_response (self, &response_copy);
    zchunk_t *request_copy = zchunk_dup (request);
    ymsg_set_request (self, &request_copy);
    return ymsg_encode (&self);
}


//  --------------------------------------------------------------------------
//  Send the SEND to the socket in one step

int
ymsg_send_send (
    void *output,
    byte version,
    uint16_t seq,
    zhash_t *aux,
    zchunk_t *request)
{
    ymsg_t *self = ymsg_new (YMSG_SEND);
    ymsg_set_version (self, version);
    ymsg_set_seq (self, seq);
    zhash_t *aux_copy = zhash_dup (aux);
    ymsg_set_aux (self, &aux_copy);
    zchunk_t *request_copy = zchunk_dup (request);
    ymsg_set_request (self, &request_copy);
    return ymsg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Send the REPLY to the socket in one step

int
ymsg_send_reply (
    void *output,
    byte version,
    uint16_t seq,
    uint16_t rep,
    zhash_t *aux,
    zchunk_t *response,
    zchunk_t *request)
{
    ymsg_t *self = ymsg_new (YMSG_REPLY);
    ymsg_set_version (self, version);
    ymsg_set_seq (self, seq);
    ymsg_set_rep (self, rep);
    zhash_t *aux_copy = zhash_dup (aux);
    ymsg_set_aux (self, &aux_copy);
    zchunk_t *response_copy = zchunk_dup (response);
    ymsg_set_response (self, &response_copy);
    zchunk_t *request_copy = zchunk_dup (request);
    ymsg_set_request (self, &request_copy);
    return ymsg_send (&self, output);
}


//  --------------------------------------------------------------------------
//  Duplicate the ymsg message

ymsg_t *
ymsg_dup (ymsg_t *self)
{
    if (!self)
        return NULL;
        
    ymsg_t *copy = ymsg_new (self->id);
    if (self->routing_id)
        copy->routing_id = zframe_dup (self->routing_id);
    switch (self->id) {
        case YMSG_SEND:
            copy->version = self->version;
            copy->seq = self->seq;
            copy->aux = self->aux? zhash_dup (self->aux): NULL;
            copy->request = self->request? zchunk_dup (self->request): NULL;
            break;

        case YMSG_REPLY:
            copy->version = self->version;
            copy->seq = self->seq;
            copy->rep = self->rep;
            copy->aux = self->aux? zhash_dup (self->aux): NULL;
            copy->response = self->response? zchunk_dup (self->response): NULL;
            copy->request = self->request? zchunk_dup (self->request): NULL;
            break;

    }
    return copy;
}


//  --------------------------------------------------------------------------
//  Print contents of message to stdout

void
ymsg_print (ymsg_t *self)
{
    assert (self);
    switch (self->id) {
        case YMSG_SEND:
            zsys_debug ("YMSG_SEND:");
            zsys_debug ("    version=%ld", (long) self->version);
            zsys_debug ("    seq=%ld", (long) self->seq);
            zsys_debug ("    aux=");
            if (self->aux) {
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->aux), item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    request=[ ... ]");
            break;
            
        case YMSG_REPLY:
            zsys_debug ("YMSG_REPLY:");
            zsys_debug ("    version=%ld", (long) self->version);
            zsys_debug ("    seq=%ld", (long) self->seq);
            zsys_debug ("    rep=%ld", (long) self->rep);
            zsys_debug ("    aux=");
            if (self->aux) {
                char *item = (char *) zhash_first (self->aux);
                while (item) {
                    zsys_debug ("        %s=%s", zhash_cursor (self->aux), item);
                    item = (char *) zhash_next (self->aux);
                }
            }
            else
                zsys_debug ("(NULL)");
            zsys_debug ("    response=[ ... ]");
            zsys_debug ("    request=[ ... ]");
            break;
            
    }
}


//  --------------------------------------------------------------------------
//  Get/set the message routing_id

zframe_t *
ymsg_routing_id (ymsg_t *self)
{
    assert (self);
    return self->routing_id;
}

void
ymsg_set_routing_id (ymsg_t *self, zframe_t *routing_id)
{
    if (self->routing_id)
        zframe_destroy (&self->routing_id);
    self->routing_id = zframe_dup (routing_id);
}


//  --------------------------------------------------------------------------
//  Get/set the ymsg id

int
ymsg_id (ymsg_t *self)
{
    assert (self);
    return self->id;
}

void
ymsg_set_id (ymsg_t *self, int id)
{
    self->id = id;
}

//  --------------------------------------------------------------------------
//  Return a printable command string

const char *
ymsg_command (ymsg_t *self)
{
    assert (self);
    switch (self->id) {
        case YMSG_SEND:
            return ("SEND");
            break;
        case YMSG_REPLY:
            return ("REPLY");
            break;
    }
    return "?";
}

//  --------------------------------------------------------------------------
//  Get/set the version field

byte
ymsg_version (ymsg_t *self)
{
    assert (self);
    return self->version;
}

void
ymsg_set_version (ymsg_t *self, byte version)
{
    assert (self);
    self->version = version;
}


//  --------------------------------------------------------------------------
//  Get/set the seq field

uint16_t
ymsg_seq (ymsg_t *self)
{
    assert (self);
    return self->seq;
}

void
ymsg_set_seq (ymsg_t *self, uint16_t seq)
{
    assert (self);
    self->seq = seq;
}


//  --------------------------------------------------------------------------
//  Get the aux field without transferring ownership

zhash_t *
ymsg_aux (ymsg_t *self)
{
    assert (self);
    return self->aux;
}

//  Get the aux field and transfer ownership to caller

zhash_t *
ymsg_get_aux (ymsg_t *self)
{
    zhash_t *aux = self->aux;
    self->aux = NULL;
    return aux;
}

//  Set the aux field, transferring ownership from caller

void
ymsg_set_aux (ymsg_t *self, zhash_t **aux_p)
{
    assert (self);
    assert (aux_p);
    zhash_destroy (&self->aux);
    self->aux = *aux_p;
    *aux_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get/set a value in the aux dictionary

const char *
ymsg_aux_string (ymsg_t *self, const char *key, const char *default_value)
{
    assert (self);
    const char *value = NULL;
    if (self->aux)
        value = (const char *) (zhash_lookup (self->aux, key));
    if (!value)
        value = default_value;

    return value;
}

uint64_t
ymsg_aux_number (ymsg_t *self, const char *key, uint64_t default_value)
{
    assert (self);
    uint64_t value = default_value;
    char *string = NULL;
    if (self->aux)
        string = (char *) (zhash_lookup (self->aux, key));
    if (string)
        value = atol (string);

    return value;
}

void
ymsg_aux_insert (ymsg_t *self, const char *key, const char *format, ...)
{
    //  Format into newly allocated string
    assert (self);
    va_list argptr;
    va_start (argptr, format);
    char *string = zsys_vprintf (format, argptr);
    va_end (argptr);

    //  Store string in hash table
    if (!self->aux) {
        self->aux = zhash_new ();
        zhash_autofree (self->aux);
    }
    zhash_update (self->aux, key, string);
    free (string);
}

size_t
ymsg_aux_size (ymsg_t *self)
{
    return zhash_size (self->aux);
}


//  --------------------------------------------------------------------------
//  Get the request field without transferring ownership

zchunk_t *
ymsg_request (ymsg_t *self)
{
    assert (self);
    return self->request;
}

//  Get the request field and transfer ownership to caller

zchunk_t *
ymsg_get_request (ymsg_t *self)
{
    zchunk_t *request = self->request;
    self->request = NULL;
    return request;
}

//  Set the request field, transferring ownership from caller

void
ymsg_set_request (ymsg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->request);
    self->request = *chunk_p;
    *chunk_p = NULL;
}


//  --------------------------------------------------------------------------
//  Get/set the rep field

uint16_t
ymsg_rep (ymsg_t *self)
{
    assert (self);
    return self->rep;
}

void
ymsg_set_rep (ymsg_t *self, uint16_t rep)
{
    assert (self);
    self->rep = rep;
}


//  --------------------------------------------------------------------------
//  Get the response field without transferring ownership

zchunk_t *
ymsg_response (ymsg_t *self)
{
    assert (self);
    return self->response;
}

//  Get the response field and transfer ownership to caller

zchunk_t *
ymsg_get_response (ymsg_t *self)
{
    zchunk_t *response = self->response;
    self->response = NULL;
    return response;
}

//  Set the response field, transferring ownership from caller

void
ymsg_set_response (ymsg_t *self, zchunk_t **chunk_p)
{
    assert (self);
    assert (chunk_p);
    zchunk_destroy (&self->response);
    self->response = *chunk_p;
    *chunk_p = NULL;
}



//  --------------------------------------------------------------------------
//  Selftest

int
ymsg_test (bool verbose)
{
    printf (" * ymsg: ");

    //  Silence an "unused" warning by "using" the verbose variable
    if (verbose) {;}

    //  @selftest
    //  Simple create/destroy test
    ymsg_t *self = ymsg_new (0);
    assert (self);
    ymsg_destroy (&self);

    //  Create pair of sockets we can send through
    zsock_t *input = zsock_new (ZMQ_ROUTER);
    assert (input);
    zsock_connect (input, "inproc://selftest-ymsg");

    zsock_t *output = zsock_new (ZMQ_DEALER);
    assert (output);
    zsock_bind (output, "inproc://selftest-ymsg");

    //  Encode/send/decode and verify each message type
    int instance;
    ymsg_t *copy;
    self = ymsg_new (YMSG_SEND);
    
    //  Check that _dup works on empty message
    copy = ymsg_dup (self);
    assert (copy);
    assert (ymsg_id(copy) == ymsg_id(self));
    ymsg_destroy (&copy);

    ymsg_set_version (self, 123);
    ymsg_set_seq (self, 123);
    ymsg_aux_insert (self, "Name", "Brutus");
    ymsg_aux_insert (self, "Age", "%d", 43);
    zchunk_t *send_request = zchunk_new ("Captcha Diem", 12);
    ymsg_set_request (self, &send_request);
    //  Send twice from same object
    ymsg_send_again (self, output);
    ymsg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = ymsg_recv (input);
        assert (self);
        assert (ymsg_routing_id (self));
        
        assert (ymsg_version (self) == 123);
        assert (ymsg_seq (self) == 123);
        assert (ymsg_aux_size (self) == 2);
        assert (streq (ymsg_aux_string (self, "Name", "?"), "Brutus"));
        assert (ymsg_aux_number (self, "Age", 0) == 43);
        assert (memcmp (zchunk_data (ymsg_request (self)), "Captcha Diem", 12) == 0);
        ymsg_destroy (&self);
    }
    self = ymsg_new (YMSG_REPLY);
    
    //  Check that _dup works on empty message
    copy = ymsg_dup (self);
    assert (copy);
    assert (ymsg_id(copy) == ymsg_id(self));
    ymsg_destroy (&copy);

    ymsg_set_version (self, 123);
    ymsg_set_seq (self, 123);
    ymsg_set_rep (self, 123);
    ymsg_aux_insert (self, "Name", "Brutus");
    ymsg_aux_insert (self, "Age", "%d", 43);
    zchunk_t *reply_response = zchunk_new ("Captcha Diem", 12);
    ymsg_set_response (self, &reply_response);
    zchunk_t *reply_request = zchunk_new ("Captcha Diem", 12);
    ymsg_set_request (self, &reply_request);
    //  Send twice from same object
    ymsg_send_again (self, output);
    ymsg_send (&self, output);

    for (instance = 0; instance < 2; instance++) {
        self = ymsg_recv (input);
        assert (self);
        assert (ymsg_routing_id (self));
        
        assert (ymsg_version (self) == 123);
        assert (ymsg_seq (self) == 123);
        assert (ymsg_rep (self) == 123);
        assert (ymsg_aux_size (self) == 2);
        assert (streq (ymsg_aux_string (self, "Name", "?"), "Brutus"));
        assert (ymsg_aux_number (self, "Age", 0) == 43);
        assert (memcmp (zchunk_data (ymsg_response (self)), "Captcha Diem", 12) == 0);
        assert (memcmp (zchunk_data (ymsg_request (self)), "Captcha Diem", 12) == 0);
        ymsg_destroy (&self);
    }

    zsock_destroy (&input);
    zsock_destroy (&output);
    //  @end

    printf ("OK\n");
    return 0;
}
