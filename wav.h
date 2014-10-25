#ifndef WAV_H
#define WAV_H

/**
From https://ccrma.stanford.edu/courses/422/projects/WaveFormat/

Offset  Size  Name             Description

The canonical WAVE format starts with the RIFF header:

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk
                               following this number.  This is the size of the
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this
                               number.
44        *   Data             The actual sound data.


*/


#define WAV_HDR_CHUNK_ID 0x46464952
#define WAV_HDR_FORMAT 0x45564157
#define WAV_HDR_SUBCHUNK1_ID 0x20746d66
#define WAV_HDR_SUBCHUNK2_ID 0x61746164

typedef struct
{
  unsigned int       ChunkID;         //  Contains the letters "RIFF" in ASCII form (0x52494646 big-endian form).
  unsigned int       ChunkSize;       //  36 + SubChunk2Size, or more precisely: 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                             //  This is the size of the rest of the chunk following this number.  This is the size of the
                             //  entire file in bytes minus 8 bytes for the two fields not included in this count: ChunkID and ChunkSize.
  unsigned int       Format;          //  Contains the letters "WAVE" (0x57415645 big-endian form).
  unsigned int       Subchunk1ID;     //  Contains the letters "fmt " (0x666d7420 big-endian form).
  unsigned int       Subchunk1Size;   //  16 for PCM.  This is the size of the rest of the Subchunk which follows this number.
  unsigned short int AudioFormat;     //  PCM = 1 (i.e. Linear quantization) Values other than 1 indicate some form of compression.
  unsigned short int NumChannels;     //  Mono = 1, Stereo = 2, etc.
  unsigned int       SampleRate;      //  8000, 44100, etc.
  unsigned int       ByteRate;        //  == SampleRate * NumChannels * BitsPerSample/8
  unsigned short int BlockAlign;      //  == NumChannels * BitsPerSample/8. The number of bytes for one sample including all channels.
  unsigned short int BitsPerSample;   //  8 bits = 8, 16 bits = 16, etc.
  /* WARNING: no PCM wav may have extra data at this point*/
  unsigned int       Subchunk2ID;     //  Contains the letters "data" (0x64617461 big-endian form).
  unsigned int       Subchunk2Size;   //  == NumSamples * NumChannels * BitsPerSample/8. This is the number of bytes in the data.

  unsigned int  fileSize(void)         { return ChunkSize + 8;}
  unsigned int  dataSize(void)         { return Subchunk2Size;}

  bool validChunkID(void)     { return ChunkID     == WAV_HDR_CHUNK_ID;     }
  bool validFormat(void)      { return Format      == WAV_HDR_FORMAT;       }
  bool validSubchunk1ID(void) { return Subchunk1ID == WAV_HDR_SUBCHUNK1_ID; }
  bool validSubchunk2ID(void) { return Subchunk2ID == WAV_HDR_SUBCHUNK2_ID; }
  bool isPCM(void)            { return AudioFormat == 1 && Subchunk1Size==16; }
  bool is8bit(void)           { return BitsPerSample==8;}
  bool isMono(void)           { return NumChannels == 1;}

  bool valid(void) { return validChunkID() && validFormat() && validSubchunk1ID() && validSubchunk2ID();}


} wav_hdr_t;


#endif // WAV_H