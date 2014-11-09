/**

  Here we define a communication protocol between Qt client program and
  Device program.
  This file is meant to be included in both projects and that is why
  it is C/C++ compatible.

  Communication will be asyncronous and with a master-request/slave-response style.
  Eg.: Qt client will send a request message to the device and will remain
  waiting for an async response or timeout exception.
  A message is completed upon response arrival.

  Both Qt client and Device should implement a state machine for handling the communication status.

  //from (http://stackoverflow.com/questions/17073302/what-type-of-framing-to-use-in-serial-communication)
  Message Frame:
  ------------------
    *   #   SOF        // start of frame
    *   #   msg_struct // the message structure
    *   #   checksum  // checksum: is a XOR of msg_struct
    *   #   EOF       // end of frame

  Message Struct:
  ------------------
    *   #   length      // length of message struct: frame - (SOF + checksum + EOF)
    *   #   msg_id      // message id, for matching requests and responses
    *   #   msg_type    // message type that matches a msg_type enum
    *   #   is_response // whether this message is a request or a response
    *   #   data        // command, status_id, struct, filename, binary data, etc
    *
    * 'data' is a uint8_t array pointer
    * this allows to have a single message structure and simplify data serialization
    * for transmission and reception.
    *
    * 'data' pointer wont be sent. instead, it will be replaced be the serialized data.
    * qt client and device are responsible for this handling
    *

  Handshaking:
  ------------
    * Handshaking is done with a handshake message.
    * A successful connection with the Device is established after with a succesful handshake message response.
    * A message (other than a handshake) should not be sent before a successful connection is established.

  Checksum:
  ---------
    * After a message, a checksum is sent, being its value a byte XOR of the message struct and data.
    * The checksum will always be the byte before EOF of messages for easing checksum verification.
    * A checksum verification should be made upon message arrival.

  Message ID:
  -----------
    * msg_id will always be the second byte of a message
    * A Request message will have a msg_id to identify it.
    * A Response message will have a msg_id identifying what request it is responding.
    * The Qt client should hold a reference of which msg_id are in use and release them
      when the corresponding response has arrived.
    * This limits up to sizeof(msg_id) simultaneous messsages, so qt client wont enqueu a new
      message until there is space for it.

  Timeouts:
  ------------
    * A timeout exception should be implemented on qt client side for each message sent.
    * A timeout exception should be implemented on device side for receiving chuncked messages.

  Status responses:
  -----------------
    * Every response message will have status_id indicating possible errors.
    * status_id will always be the 'data' byte of a message response
    * If a chunck response return and error, qt client is responsible of resend it
      and Device is responsible of receiving it again.


  Messages size:
  --------------
    * First byte of a message indicates its length
    * A fixed length message will not exceed MAX_PACKET_SIZE size.
    * A variable length message will have a header message and data splitted in chunk messages.
    * The header itself will be fixed length, indicating data_length and chuncks_count
    * The chunk message will be  of a maximum MAX_PACKET_SIZE size, including a chunk_id (index).


  TODOs: (wont do in this version)
  ------
    * Rewrite functions to allow multiple bytes for
      START_OF_FRAME, checksum and END_OF_FRAME
    * Make the client handle memory instead of allocating memory here.



*/


#ifndef PROTOCOL_H
#define PROTOCOL_H


// macros have to be outside extern "C" block
// to be defined on other sources that includes this file
#define MAX_PACKET_SIZE 128
#define MAX_UNFRAMED_DATA 256
#define START_OF_FRAME 0xFA
#define END_OF_FRAME 0xCC
#define RAW_RX_BUFFER_SIZE 1024


/*
Multi Language Header
Allows cpp to be C-compatible
And includes either cinttypes or inttypes.h
for compatible int data types
*/
#ifdef __cplusplus
#include <cinttypes>
extern "C" {
#else
#include <inttypes.h>
#endif
#include <stdlib.h>

/*
End of Multi Language Header
*/


/*START OF C/C++ COMMON CODE - (do not code aboce this line)*/


typedef enum {
  STATUS_OK,
  STATUS_ERROR
} status_id_t;

typedef enum {
  MESSAGE_HANDSHAKE,
  MESSAGE_INFO_STATUS,
  MESSAGE_PLAYBACK_COMMAND,
  MESSAGE_FILEHEADER,
  MESSAGE_FILECHUNK,
  MESSAGE_MAX_VALID_TYPE,
} message_type_t;

typedef enum{
  BUFFER_NOT_SOF, // not start of frame
  BUFFER_SOF,     // start of frame
  BUFFER_IN_MSG,  // in message
  BUFFER_EOF,     // end of frame
  BUFFER_MSG_OK,  // message is ready to pop
  BUFFER_ERROR_SOF_EXPECTED,
  BUFFER_ERROR_CHECKSUM,
  BUFFER_ERROR_EOF_EXPECTED,
  BUFFER_ERROR_INVALID_MSG_LENGTH,
} buffer_status_t;

typedef struct
{
  uint8_t  length;
  uint8_t  msg_id;
  union{
    uint8_t  msg_full_type; //for serialization purposes
    struct{
      uint8_t  is_response:1;
      uint8_t  msg_type:7;
    };
  };
  uint8_t* data;
} message_t;

typedef struct
{
  u_int32_t filesize;
  u_int32_t sample_rate;
  u_int32_t  chunks_count;
  char     filename[8];
} fileheader_data_t;

typedef struct
{
  u_int32_t total_space;
  u_int32_t available_space;
  uint8_t  sd_connected;
  uint8_t  files_count;
} status_data_t;

typedef struct
{
  u_int32_t  chunk_id;
  uint8_t*  data;
} filechunk_data_t;






//utility functions:
uint8_t messageGetChecksum(message_t* message);
buffer_status_t messagesBufferProcess ( void);
void messagesBufferPush ( uint8_t data );
uint8_t* messagesBufferPop( void);




/*END OF C/C++ COMMON CODE - (do not code below this line)*/

/*
Close c++ bracket for Multi Language Header
*/
#ifdef __cplusplus
}
#endif
/*
End of Close c++ bracket for Multi Language Header
*/


#endif // PROTOCOL_H
