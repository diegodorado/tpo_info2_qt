/*
Multi Language Source
Allows cpp to be C-compatible
And includes either cinttypes or inttypes.h
for compatible int data types
*/

#ifdef __cplusplus____
extern "C" {
#else
#endif
/*
End of Multi Language Header
*/


/*START OF C/C++ COMMON CODE - (do not code above this line)*/
#include "protocol.h"


static uint8_t raw_rx_buffer[RAW_RX_BUFFER_SIZE];
static int raw_rx_buffer_in_index = 0;
static int raw_rx_buffer_out_index = 0;
static int unframed_data_count = 0;
static buffer_status_t buffer_status = BUFFER_NOT_SOF; //initial buffer state

//static functions prototypes
static uint8_t validate_buffer_checksum();
static int validate_end_of_frame();
static uint16_t buffered_message_data_length();
static int buffered_message_length();
static uint8_t raw_rx_buffer_at(int i);
static int raw_rx_buffer_pos(int i);
static int raw_rx_buffer_count();


uint8_t messageGetChecksum(message_hdr_t* message, uint8_t* data)
{
  uint8_t result = 0;
  uint16_t i;

  for(i = 0; i < sizeof(message_hdr_t); i++)
    result ^= *( ((uint8_t*) message) +i);

  for(i = 0; i < message->data_length; i++)
    result ^= *(data+i);

  return result;
}



//todo: check if buffer is not full
void messagesBufferPush ( uint8_t data )
{
  raw_rx_buffer[raw_rx_buffer_in_index] = data;
  raw_rx_buffer_in_index++;
  raw_rx_buffer_in_index %= RAW_RX_BUFFER_SIZE;
}



/*
 * return a message pointer
 *
 * warning: data will not be a pointer but data actually!
 * it may return null in two cases:
 *  a) buffer_status!=BUFFER_MSG_OK
 *  b) malloc returns NULL
 *
 * NOTE: be aware that this function allocates memory
 * and you are responsible for freeing it
*/
uint8_t* messagesBufferPop ( void)
{
  uint8_t* raw_data = NULL;
  uint16_t i;
  uint16_t l = buffered_message_length();

  if (buffer_status==BUFFER_MSG_OK){
    raw_data = (uint8_t*) malloc (l*sizeof(uint8_t));
    if (raw_data ==NULL)
    {
      buffer_status = BUFFER_NOT_SOF;
    }
    else
    {
      for(i=0;i<l;i++){
        *(raw_data+i) = raw_rx_buffer[raw_rx_buffer_out_index++];
        raw_rx_buffer_out_index %= RAW_RX_BUFFER_SIZE;
      }
      // raw_rx_buffer_out_index points to checksum now...
      //so, advance the buffer after checksum and eof bytes
      raw_rx_buffer_out_index += 2;
      raw_rx_buffer_out_index %= RAW_RX_BUFFER_SIZE;
      buffer_status = BUFFER_NOT_SOF;
    }
  }
  return raw_data;
}


void messagesBufferClear ()
{
  raw_rx_buffer_out_index = raw_rx_buffer_in_index;
  buffer_status = BUFFER_NOT_SOF;
}

static int raw_rx_buffer_count()
{
  return ( RAW_RX_BUFFER_SIZE + raw_rx_buffer_in_index - raw_rx_buffer_out_index ) % RAW_RX_BUFFER_SIZE;
}

/*
 * returns buffer index
 * being raw_rx_buffer_out_index the 0th element
*/
static int raw_rx_buffer_pos(int i)
{
  return ( raw_rx_buffer_out_index + i ) % RAW_RX_BUFFER_SIZE;
}

/*
 * returns the i-th element of the buffer
 * being raw_rx_buffer_out_index the 0th element
*/
static uint8_t raw_rx_buffer_at(int i)
{
  return raw_rx_buffer[raw_rx_buffer_pos(i)];
}


/*
 * returns the message length
 * caution! only valid if buffer status
 * is BUFFER_IN_MSG or BUFFER_EOF
*/
static uint16_t buffered_message_data_length()
{

  uint8_t i;
  uint16_t length;

  if (buffer_status!=BUFFER_EOF
      && buffer_status!=BUFFER_IN_MSG
      && buffer_status!=BUFFER_MSG_OK)
    return 0;

  for(i=0;i<sizeof(uint16_t);i++)
    *( ((uint8_t*) &length )+i) = raw_rx_buffer_at(i);

  //msg length is the first two bytes
  return length;
}


static int buffered_message_length()
{
  return sizeof(message_hdr_t) + buffered_message_data_length();
}


/*
 * validates EOF of the buffer
 * caution! only valid if buffer status
 * is BUFFER_EOF
 * returns 1 if valid EOF, 0 otherwise
*/
static int validate_end_of_frame()
{

  return raw_rx_buffer_at(buffered_message_length()+1) == END_OF_FRAME;
}

/*
 * validates checksum of the buffer
 * caution! only valid if buffer status
 * is BUFFER_EOF
 * returns 1 if valid checksum, 0 otherwise
 * note: differs from validateChecksum from being
 * a raw checksum (not a message_t struct but an array)
*/

static uint8_t validate_buffer_checksum()
{
  uint8_t calculated_checksum = 0;
  int i;

  if (buffer_status!=BUFFER_EOF)
    return 0;

  for(i = 0; i < buffered_message_length() ; i++)
    calculated_checksum ^= raw_rx_buffer_at(i);

  return raw_rx_buffer_at(buffered_message_length()) == calculated_checksum;
}




/*
 * processes the buffer in search for a valid message
 * returns a buffer status
 *
 * if BUFFER_MSG_OK is returned, only then message is ready to pop from buffer
 * note: function wont have any effect if status==BUFFER_MSG_OK until
 * the message is poped from buffer
*/
buffer_status_t messagesBufferProcess ( void)
{

  //check if an error ocurred last time
  if(buffer_status==BUFFER_ERROR_SOF_EXPECTED
     || buffer_status==BUFFER_ERROR_INVALID_MSG_LENGTH
     || buffer_status==BUFFER_ERROR_EOF_EXPECTED
     || buffer_status==BUFFER_ERROR_CHECKSUM )
  {
    //an error ocurred before... then, clear the buffer and reset status
    messagesBufferClear();
  }


  if(buffer_status==BUFFER_NOT_SOF){
    while(raw_rx_buffer_count()>0 && raw_rx_buffer_at(0) != START_OF_FRAME)
    {
      raw_rx_buffer_out_index++;
      unframed_data_count++;
    }

    if (raw_rx_buffer_at(0) == START_OF_FRAME)
    {
      unframed_data_count = 0;
      raw_rx_buffer[raw_rx_buffer_out_index] = 0; //read it only once
      raw_rx_buffer_out_index++; //discard SOF byte
      buffer_status = BUFFER_SOF;
    }
    else
    {
      if(unframed_data_count>MAX_UNFRAMED_DATA)
        // too much data buffered without a start of frame byte!
        buffer_status = BUFFER_ERROR_SOF_EXPECTED;
    }
  }

  if(buffer_status==BUFFER_SOF) {
    if(raw_rx_buffer_count()>=2)
      // buffer count should at least be 2 to read message length
      buffer_status = BUFFER_IN_MSG;
  }

  if(buffer_status==BUFFER_IN_MSG) {
    if(raw_rx_buffer_count() >= buffered_message_length()  + 2)
      //if buffer length is more than message header + data length + checksum byte + eof byte
      //then, frame should be ended
      buffer_status = BUFFER_EOF;
  }

  if(buffer_status==BUFFER_EOF) {
    //we have a full message buffered
    //let's validate checksum and eof byte


    if(!validate_end_of_frame())
    {
      // seems message lacks of EOF where expected
      buffer_status = BUFFER_ERROR_EOF_EXPECTED;
    }
    else
    {
      //only validate checksum if EOF is valid
      if(!validate_buffer_checksum())
      {
        //checksum sent and calculated does not match!
        buffer_status = BUFFER_ERROR_CHECKSUM;
      }
      else{
        //valid EOF and checksum
        //message is ready to pop!
        buffer_status = BUFFER_MSG_OK;
      }
    }

  }

  //return the process status
  return buffer_status;
}

uint8_t* messageData(message_hdr_t* message)
{
  return (uint8_t*) message + sizeof(message_hdr_t) ;
}



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




