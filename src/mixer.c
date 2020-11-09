/*****************************************************************************/
/* mixer.c v0.1                Sample Mixer                                  */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Thu Apr  4 12:57:17 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef PLAT_LINUX
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#else
#include "portaudio.h"
#endif
#include "mixer.h"
#include "mod.h"


/* Variable declarations/definitions used only by the mixer.  These are      */
/* guaranteed to be initialized to 0                                         */
#ifdef PLAT_LINUX
static int dspfd;
#else
PaStream *stream = NULL;
#endif

static struct
{
  uint32* buffer;
  /* a 32bit wide buffer is used so that carry bits are kept when adding 16  */
  /* bit samples.  Only after all the channels have submitted is the         */
  /* division done.  NB with 32 16bit channels 21 bits could be used max.    */
  int bufsize;                  /* bufsize in number of 32bit slots, !bytes  */
  uint16* flushbuf;
  int blocksize;
  uint8  num_lchan;
  uint8  num_rchan;
  uint8  num_channels;
  uint8  stereo;
  uint8  res;
  int rate;
  int left;
  int right;
  int in[MIXER_MAX_CHANNELS];   /* an array of indexes into the buffer       */
  int out;
} mixer;

static int divtable[] = { 0, 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };
  
static int FlushAvail(void);
static int WriteAvail(int channel_index);



MPstatus MixerChangeRate(int * new_rate)
{
  /* call the flush buffer command */
  /* call the sync sound card command */
  /* change the rate with the soundcard via ioctl.  Then update *new_rate    */
  /* and the global rate                                                     */
  /* NOTE: a click may be heard for the changexxxx operations.  No need to   */
  /* implement them yet.  They will eventually be used inbetween playing     */
  /* different mod files.                                                    */
  return(MP_OK);
}



MPstatus MixerChangeResolution(int * new_resolution)
{
  /* similar to ChangeRate */
  return(MP_OK);
}



int MixerGetRate(void)
{
  return(mixer.rate);
}



/* If either left or right contains no channels the mixer should initialize  */
/* to mono mode.  This function MUST be called prior to using the mixer.     */
/* The actual rate parameter used will be returned in *rate                  */
MPstatus MixerInitialize(int rate, int resolution, int left, int right)
{
  #ifndef PLAT_LINUX
  PaStreamParameters outputParameters;
  PaError err;
  //char *sampleBlock;
  int n, mode;
  //int numBytes;
  printf("Port Audio Initialize...\n"); fflush(stdout);

  err = Pa_Initialize();
  if (err != paNoError)
	  return(MP_UNAVAIL_DSP);

  if ((resolution == 8) || (resolution == 16))
    mixer.res = resolution;
  else
    return(MP_DSP_BADARG);

  outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
  printf("Device %d\n", outputParameters.device);
  outputParameters.channelCount = 1; //NUM_CHANNELS;
  outputParameters.sampleFormat = paInt16; /*resolution == 16 ? paInt16 : paUInt8;*/ //PA_SAMPLE_TYPE;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
  outputParameters.hostApiSpecificStreamInfo = NULL;
  if ((resolution == 8) || (resolution == 16))
    mixer.res = resolution;
  else
    return(MP_DSP_BADARG);

  /* duplicate channels on either side OR no channels listed                 */
  if ((left & right) || (!(left | right)))
    return(MP_DSP_BADARG);

  mixer.right = right;
  mixer.left = left;
  if (left && right) 
    mode = STEREO;
  else
    mode = MONO;
  mixer.stereo = mode;
  mixer.rate = rate;
  mixer.num_lchan = mixer.num_rchan = 0;
  for (n = 0; n < MIXER_MAX_CHANNELS; n++)
  {
    mixer.num_lchan += (left & 0x1);
    mixer.num_rchan += (right & 0x1);
    left >>= 1;
    right >>= 1;
  }
  mixer.num_channels = mixer.num_rchan + mixer.num_lchan;
  mixer.bufsize = MIXER_MAX_BUFFER;

  /* Number of 16bit samples. */
  mixer.blocksize = MIXER_INTERNAL_BUFFER;
  if (!(mixer.flushbuf = (int16*) malloc( MIXER_INTERNAL_BUFFER*2 )))
    return(MP_NOMEM);

  /* It is better to use calloc here since we need the space initialized */
  if (!(mixer.buffer = (uint32*) calloc( mixer.bufsize, sizeof(uint32))))
    return(MP_NOMEM);

  /* initialize buffer pointers (well, they're actually just indexes)    */
  for (n = 0; n < mixer.num_channels; n++)
    mixer.in[n] = 0;

  mixer.out = 0;

  err = Pa_OpenStream
  (
    &stream,
    NULL,
    &outputParameters,
    rate,
    MIXER_INTERNAL_BUFFER/2,
    paClipOff,
    NULL,
    NULL
  );

  if (err != paNoError)
    return(MP_UNAVAIL_DSP);

  err = Pa_StartStream(stream);
  if (err != paNoError)
    return(MP_UNAVAIL_DSP);

  return(MP_OK);

#else
  int drv_block_size, n, mode;
  int real_res = 16;
  if ((dspfd = open(DSP_DEV, O_WRONLY | O_SYNC, 0)) == -1)
    return(MP_UNAVAIL_DSP);

  /* Note: the actual resolution used by the mixer will always be 16 bits    */
  if (ioctl(dspfd, SNDCTL_DSP_SAMPLESIZE, &real_res) == -1)
    return(MP_DSP_ERROR);

  if ((resolution == 8) || (resolution == 16))
    mixer.res = resolution;
  else
    return(MP_DSP_BADARG);

  /* duplicate channels on either side OR no channels listed                 */
  if ((left & right) || (!(left | right)))
    return(MP_DSP_BADARG);

  mixer.right = right;
  mixer.left = left;

  if (left && right) 
    mode = STEREO;
  else
    mode = MONO;

  if (ioctl(dspfd, SNDCTL_DSP_STEREO, &mode) == -1)
      return(MP_DSP_ERROR);
  mixer.stereo = mode;

  if (ioctl(dspfd, SNDCTL_DSP_SPEED, &rate) == -1)
    return(MP_DSP_ERROR);
  mixer.rate = rate;

  mixer.num_lchan = mixer.num_rchan = 0;
  for (n = 0; n < MIXER_MAX_CHANNELS; n++)
  {
    mixer.num_lchan += (left & 0x1);
    mixer.num_rchan += (right & 0x1);
    left >>= 1;
    right >>= 1;
  }
  mixer.num_channels = mixer.num_rchan + mixer.num_lchan;

  if ((ioctl(dspfd, SNDCTL_DSP_GETBLKSIZE, &drv_block_size) == -1) || (drv_block_size < 2))
    return(MP_DSP_ERROR);

  /* Presume returned block size is in bytes and is divisible by 2       */
  /* since we are storing the number of 16bit samples, i.e. 2 bytes each */
  mixer.blocksize = drv_block_size / 2;

  if (!(mixer.flushbuf = (int16*) malloc( drv_block_size )))
    return(MP_NOMEM);

  if ((n = 2 * drv_block_size) > MIXER_MAX_BUFFER)
    mixer.bufsize = n;
  else
  {
    while ((n += drv_block_size) <= MIXER_MAX_BUFFER);
    mixer.bufsize = n - (drv_block_size);
  }

  /* It is better to use calloc here since we need the space initialized */
  if (!(mixer.buffer = (uint32*) calloc( mixer.bufsize, sizeof(uint32))))
    return(MP_NOMEM);

  /* initialize buffer pointers (well, they're actually just indexes)    */
  for (n = 0; n < mixer.num_channels; n++)
    mixer.in[n] = 0;

  mixer.out = 0;
  return(MP_OK);
#endif
}



/* Linear search for the index of the next channel after 'out'           */
/* The amount available to flush is returned                             */
int FlushAvail(void)
{
  int n, lowest, cdiff, diff;

  lowest = 0;
  cdiff = mixer.bufsize;
  for (n = 0; n < mixer.num_channels; n++)
  {
    diff = (mixer.in[n] < mixer.out) ? 
      mixer.bufsize - mixer.out + mixer.in[n] : mixer.in[n] - mixer.out;
    if (diff < cdiff)
      cdiff = diff;
  }
  return(cdiff);
}



/* Amount avail to write to channel (ch_index+1)                         */
/* Note: out == in        => empty                                       */
/*       out == in+1      => full                                        */
/* returns the number of samples that can be written... so it needs to   */
/* look and see if it's stereo or not                                    */
int WriteAvail(int ch_index)
{
  int in = mixer.in[ch_index];
  int out = mixer.out;

  out = (out > in) ?
    (out - in - 1) :
    (mixer.bufsize - in + out - 1);
  
  return(out >> mixer.stereo);
}



/* goes from a channel ID to channel index.  Is this too slow?... should */
/* I require the user to supply the channel number (= index + 1)         */
int MixerGetChanIndex(int channel_id)
{
  int index = 0;
  while (channel_id >>= 1) index++;
  return(index);
}



int MixerGetChanID(int channel_index)
{
  return(1 << channel_index);
}



/* length should contain the number of individual samples (be they 8 or  */
/* 16 bit).  The return value of length is the number of samples that    */
/* STILL NEED TO BE WRITTEN... not the number written (it's more useful) */
/* void** is chosen for the samples pointer since as the data may be 8 or*/
/* 16 bit. *void is incremented by the number of samples mixed on return */
int Mix(int channel, uint8** samp_ptr, int length)
{
  int length_avail, side, n, buf_add, index;
  int ch_index  = MixerGetChanIndex(channel);
  void* samples = *samp_ptr;
  uint8* samp8  = (uint8*) samples;
  int16* samp16 = (int16*) samples;

  side = ((channel & mixer.right) && mixer.stereo) ? 1 : 0;
  buf_add = mixer.in[ch_index] + side;
  length_avail = WriteAvail(ch_index);
  if (length_avail > length) length_avail = length;

  for (n = 0; n < length_avail; n++)
  {
    index = ((n << mixer.stereo) + buf_add) % mixer.bufsize;
    mixer.buffer[index] += 
      (mixer.res >> 4) ? *samp16++ : *samp8++;    
  }
  mixer.in[ch_index] = (mixer.in[ch_index] +
                       (length_avail << mixer.stereo)) % mixer.bufsize;

  /* if all of sample written, leave samp_ptr at the beginning          */
  /* todo: should this depend on sample size and stereo?                */
  *samp_ptr += length_avail;
  MixerFlush();
  return(length - length_avail);
}



/* Here's the code to flush the buffer. (if able)                       */
MPstatus MixerFlush(void)
{
  int length_avail, flush_blocks, nl, nr, n, s;
  int lch_lshift, lch_rshift, rch_lshift, rch_rshift;
#ifndef PLAT_LINUX
  PaError err;
#else
  int to_write, w;
#endif

  length_avail = FlushAvail();

  /* return if nothing to flush                                         */
  if (!(flush_blocks = length_avail / mixer.blocksize))
    return(MP_OK);

  /* The problem is that shifting left by a neg amount is not guaranteed*/
  /* to shift right by the absolute value                               */
  /* This is tricky, be careful.  It needs to be done like this so we   */
  /* don't lose any sig figs.                                           */
  if (mixer.stereo)
  {
    nl = mixer.num_lchan;            /* stereo */
    nr = mixer.num_rchan;
  }
  else
    nl = nr = mixer.num_channels;    /* mono */

  if ((rch_lshift = ((mixer.res == 8) ? 8 : 0) - divtable[nr]) > 0)
    rch_rshift = 0;
  else
  {
    rch_rshift = -rch_lshift;
    rch_lshift = 0;
  }
  if ((lch_lshift = ((mixer.res == 8) ? 8 : 0) - divtable[nl]) > 0)
    lch_rshift = 0;
  else
  {
    lch_rshift = -lch_lshift;
    lch_lshift = 0;
  }

  /* now one of the right or left shifts will be zero so no loss of     */
  /* significant digits.  We also have no loss if we mix 8bit data!     */
  for (n = 0; n < flush_blocks; n++)
  {
    for(s = 0; s < mixer.blocksize; s += 2)
    {
      mixer.flushbuf[s] = (mixer.buffer[(s + mixer.out) % mixer.bufsize])
                          << lch_lshift >> lch_rshift ^ 0x8000;
      mixer.buffer[(s + mixer.out) % mixer.bufsize] = 0;
    }
    for(s = 1; s < mixer.blocksize; s += 2)
    {
      mixer.flushbuf[s] = (mixer.buffer[(s + mixer.out) % mixer.bufsize]) 
                          << rch_lshift >> rch_rshift ^ 0x8000;
      mixer.buffer[(s + mixer.out) % mixer.bufsize] = 0;
    } 
#ifdef PLAT_LINUX
    to_write = mixer.blocksize << 1;          /* To get number of bytes */
    while (to_write)
    {
      if ((w = write(dspfd, mixer.flushbuf, to_write)) == -1)
/*      if ((w = write(1, mixer.flushbuf, to_write)) == -1) */
        return(MP_DSP_ERROR);
      to_write -= w;
    }
#else
    err = Pa_WriteStream( stream, mixer.flushbuf, mixer.blocksize );
    if (err != paNoError)
      return(MP_DSP_ERROR);
#endif

    mixer.out = (mixer.out + mixer.blocksize) % mixer.bufsize;
  } 
  return(MP_OK);
}



#ifdef PLAT_LINUX
MPstatus MixerPlaySamples(void)
{
  unsigned char buf[32768];
  int dsp_fd, n, w;
  uint32 m;
  int ssize, stereo, frequency;

  if ((dsp_fd = open(DSP_DEV, O_WRONLY | O_SYNC , 0)) == -1)
    return(MP_UNAVAIL_DSP);

  ssize = 8;
  ioctl(dsp_fd, SNDCTL_DSP_SAMPLESIZE, &ssize);

  stereo = 0;
  ioctl(dsp_fd, SNDCTL_DSP_STEREO, &stereo);

  frequency = 22050;
  ioctl(dsp_fd, SNDCTL_DSP_SPEED, &frequency);

  for(n=0; n<MOD_NUM_SAMPLES; n++)
  {
    printf("Sample %d  Length %d\n", n, mod.sample_desc[n].length);
    for(m=0; m<mod.sample_desc[n].length; m++) buf[m] = mod.sample[n][m]; 
    w = write(dsp_fd, buf, mod.sample_desc[n].length);
    
    printf("Sample %d  NumWritten %d\n", n, w);
  } 

  return(MP_OK);
}
#endif


