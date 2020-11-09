/*****************************************************************************/
/* mixer.h v0.1                Sample Mixer Declarations                     */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Thu Apr  4 13:01:56 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#ifndef mixer_h
#define mixer_h

#include "mod.h"

/* don't worry... the correct amount of space for any number of channels     */
/* below these limits is malloced (no more)                                  */
#define MIXER_MAX_CHANNELS 32

/* The max buffer size is not strictly adhered to.  If twice the block size  */
/* returned by the device driver is bigger than this max then twice the      */
/* block size is used instead. (i.e. a min of twice the block size)          */
/* Otherwise, the biggest multiple of twice the blksize <= to this maximum   */
/* will be used.                                                             */
#ifdef PLAT_LINUX
#define MIXER_MAX_BUFFER 32768
#else
#define MIXER_MAX_BUFFER 32768
#define MIXER_INTERNAL_BUFFER 256
#endif
/* #define MIXER_MAX_BUFFER 262144 */
#define STEREO 1
#define MONO 0
#define DEFAULT_RATE 22050
#define DEFAULT_RESOLUTION 8


/* The available channels.  More may need to be added later                  */
enum
{
  NO_CHANNEL =    0,
  CHANNEL1 =      1,
  CHANNEL2 =      2,
  CHANNEL3 =      4,
  CHANNEL4 =      8,
  CHANNEL5 =      16,
  CHANNEL6 =      32,
  CHANNEL7 =      64,
  CHANNEL8 =      128
};



/* This should eventually be inserted by the makefile */
#define DSP_DEV       "/dev/dsp"

MPstatus MixerPlaySamples(void);          /* This is just a function used for testing */
MPstatus MixerChangeRate(int* new_rate);
MPstatus MixerChangeResolution(int* new_resolution);
int      MixerGetRate(void);
int      MixerGetChanIndex(int channel_id);
int      MixerGetChanID(int channel_index);
MPstatus MixerInitialize(int rate, int res, int left, int right);
MPstatus MixerFlush(void);
int      Mix(int channel_id, void** sample_pointer, int length_in_samples);
#endif
