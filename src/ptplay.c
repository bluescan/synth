/*****************************************************************************/
/* ptplay.c v0.1           Protracker (MOD) Play Routines                    */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Thu Apr 18 03:49:09 PDT 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "ptplay.h"
#include "mod.h"
#include "mixer.h"
//#pragma optimize("", off)


/* A couple of macros to clear/set the volume and increment functions */
#define SETINCFUN(fun)   chan_state[channel].CalcCurrInc = fun
#define SETVOLFUN(fun)   chan_state[channel].CalcCurrVol = fun
#define CLEARINCVOLFUN   chan_state[channel].CalcCurrInc = NULL; \
                         chan_state[channel].CalcCurrVol = NULL
#define CLEARVOLFUN      chan_state[channel].CalcCurrVol = NULL
#define CLEARINCFUN      chan_state[channel].CalcCurrInc = NULL



struct chan_data
{
  uint8* sample;
  uint32 sample_length;
  uint32 sample_position;
  /* Interpret as a fixed point value.  The "point" is only 15 positions to  */
  /* the left as it is possible that the sample position may be a 17 bit no  */

  uint32 repeat_point;
  uint32 repeat_length;
  uint8  clear_val;
  uint8  curr_samp_vol;   /* current volume */
  uint32 curr_samp_inc;
  /* encodes the current period/freq as a fixed point no. */

  uint32 div_samp_inc;
  /* encoded this divisions period/freq effect parameter. */

  uint32 arpeggio_inc_a;
  uint32 arpeggio_inc_b;
  uint32 arpeggio_inc_c;  /* The three arpeggio frequencies i.e. inc amounts */
  int    arpeggio_one_third_div_pos;
  int    arpeggio_two_third_div_pos;
  uint32 slide_delta;     /* a 20 b.p. fixed point no. */
  uint32 slide_period;    /* a 20 b.p. fixed point no. */

  uint32 (*CalcCurrInc)(int, int);
  uint32 (*CalcCurrVol)(int, int); 
  /* points to the correct increment/volume calculating function */
};



static struct chan_data * chan_state;
static uint8 pattern,  channel, tpd, song_pos;
static int8 division; /* division may need to be -1 if a pat break occurs */
/* static uint8 pattern_break;
 A value of $FF means no pattern break, otherwise the divisions no is given*/

static uint8 * tick_buf;
static uint32 out_rate, tick_buf_size, max_div_pos;
/* tick_buf_size is the current amount of the tick_buf that is being used.   */
/* It is a function  of the output rate and the bpm setting.                 */
/* max_div_pos will be the product of the current tpd and tick_buf_size      */

/* I still need to make more of the stuff static */
static MPstatus InitializePlayer(void);
static MPstatus ResampleTick(int tick_no);
static MPstatus PlayDivision(void);



static uint32 semitone_factor[] =
{
  0x00002000, 0x000021E7, 0x000023EB, 0x0000260E,
  0x00002851, 0x00002AB7, 0x00002D41, 0x00002FF2,
  0x000032CC, 0x000035D1, 0x00003904, 0x00003C68,
  0x00004000, 0x000043CE, 0x000047D6, 0x00004C1C
};
/* This table is the set of factors that will inc a pitch by a semitone.     */
/* They are stored as 13 b.p. fixed point numbers.  The 13 is because when   */
/* multiplied by the 15 b.p. curr_samp_inc amount we have a 28 b.p. number.  */
/* The four upper bits are required in case curr_samp_inc is not just        */
/* fractional.  i.e. max val curr_samp_inc = aprox 16. worst case:           */
/* out_rate = 4000Hz, resample rate = 60KHz => curr_samp_inc = 15            */
/* The values are 2^(x/12), where x=0,1,2,...  The table saves work as well  */
/* as being more accurate.  ex. 44100Hz up 15 semitones  should be 104888.07 */
/* while the table would give 104888.01.  For smaller numbers results will be*/
/* better.                                                                   */



MPstatus InitializePlayer(void)
{
//  int n;
  int default_bpm = 125;

  out_rate = MixerGetRate();

  /* I think this is the fastest way to do this...                           */
  tick_buf_size = (5 * out_rate)/(default_bpm << 1);
  tpd = 6;
  max_div_pos = (tick_buf_size * tpd) - 1;

  /* We have to allocate the largest possible amount since the bpm rate may  */
  /* change mid-song, and the rate may change between songs.                 */
  if (!(tick_buf = (uint8*) malloc(MAX_TICK_BUFFER_SIZE)))
    return(MP_NOMEM);

  if (!(chan_state = (struct chan_data *) 
                     malloc(mod.number_channels * sizeof(struct chan_data))))
    return(MP_NOMEM);

  /* I think the only one that needs setting is sample to NULL.  Try this    */
  /* later.  For now, we reset everything.                                   */
  /* The convention is that is the sample pointer is set to NULL then the    */
  /* channel is turned off.                                                  */
  for (channel = 0; channel < mod.number_channels; channel++)
  {
    chan_state[channel].sample_position = 0;
    /* do we want to ignore frst 2 byts?*/

    chan_state[channel].sample = NULL;      /* required */
    chan_state[channel].clear_val = 0;      /* required */
    chan_state[channel].sample_length = 0;
    chan_state[channel].curr_samp_inc = 0;  /* may not be necessary but keep */
    chan_state[channel].div_samp_inc = 0;   /* may not be necessary but keep */
    chan_state[channel].curr_samp_vol = 0;  /* may not be necessary but keep */
    chan_state[channel].CalcCurrInc = NULL; /* required */
    chan_state[channel].CalcCurrVol = NULL; /* required */
    chan_state[channel].slide_delta = 0;    /* not sure if required */
    chan_state[channel].slide_delta = 0;    /* not sure if required */
  };
  return(MP_OK);
}



void ClearTickBuffer(void)
{
  uint32 t;
  for (t = 0; t < tick_buf_size; t++)
    tick_buf[t] = chan_state[channel].clear_val;
}



uint32 CalcArpeggioInc(int tick, int tick_pos)
{
  int div_pos = (tick*(int)tick_buf_size)+tick_pos;
  if (div_pos <= chan_state[channel].arpeggio_one_third_div_pos)
    return(chan_state[channel].arpeggio_inc_a);

  if (div_pos <= chan_state[channel].arpeggio_two_third_div_pos)
    return(chan_state[channel].arpeggio_inc_b);

  if (div_pos < (int)max_div_pos)
    return(chan_state[channel].arpeggio_inc_c);

  return(chan_state[channel].arpeggio_inc_a);
  /* We must be at the end of the sample if this return is reached */
}



uint32 CalcSlideUpInc(int tick, int tick_pos)
{
  if (((chan_state[channel].slide_period -= chan_state[channel].slide_delta)
                                         >> 20) < MOD_SLIDE_MIN_PER)
  {
    chan_state[channel].slide_delta = 0; /* This avoids a nasty bug I think */
    return(chan_state[channel].curr_samp_inc);
  }
  else
    return(((AMIGA_CLOCK / (chan_state[channel].slide_period >> 19)) 
             << 15) / out_rate);
}



uint32 CalcSlideDownInc(int tick, int tick_pos)
{
  if (((chan_state[channel].slide_period += chan_state[channel].slide_delta)
                                         >> 20) > MOD_SLIDE_MAX_PER)
  {
    chan_state[channel].slide_delta = 0; /* This avoids a nasty bug I think */
    return(chan_state[channel].curr_samp_inc);
  }
  else
    return(((AMIGA_CLOCK / (chan_state[channel].slide_period >> 19))
             << 15) / out_rate);
}



MPstatus ProcessEEffect(uint8 effect, uint8 x)
{
  static uint8 effect1msg = 0;
  static uint8 effect2msg = 0;
  static uint8 effect3msg = 0;
  static uint8 effect4msg = 0;
  static uint8 effect5msg = 0;
  static uint8 effect6msg = 0;
  static uint8 effect7msg = 0;

  static uint8 effect9msg = 0;
  static uint8 effectAmsg = 0;
  static uint8 effectBmsg = 0;
  static uint8 effectCmsg = 0;
  static uint8 effectDmsg = 0;
  static uint8 effectEmsg = 0;
  static uint8 effectFmsg = 0;
  switch (effect)
  {
  case 0x0:  /* Filter on/off */
    switch (x)
    {
    case 1:
      printf("Filter/Power LED off\n");
      break;
    case 0:
      printf("Filter/Power LED on\n");
      break;
    default:
      return(MP_BADEFFECT);
    }
    CLEARINCVOLFUN;
    break;

  case 0x1:  /* Fineslide up */
    if (!effect1msg)
      printf("Fineslide up Not Implemented Yet.\n");
    effect1msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x2:  /* Fineslide down */
    if (!effect2msg)
      printf("Fineslide down Not Implemented Yet.\n");
    effect2msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x3:  /* Set Glissando */
    if (!effect3msg)
      printf("Set Glissando Not Implemented Yet.\n");
    effect3msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x4:  /* Set Vibrato Waveform */
    if (!effect4msg)
      printf("Set Vibrato Waveform Not Implemented Yet.\n");
    effect4msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x5:  /* Set Finetune */
    if (!effect5msg)
      printf("Set Finetune Not Implemented Yet.\n");
    effect5msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x6:  /* Patternloop */
    if (!effect6msg)
      printf("Patternloop Not Implemented Yet.\n");
    effect6msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x7:  /* Set Tremolo Waveform */
    if (!effect7msg)
      printf("Set Tremolo Waveform Not Implemented Yet.\n");
    effect7msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0x8:  /* Invalid Effect */
    CLEARINCVOLFUN;
    return(MP_BADEFFECT);

  case 0x9:  /* Retrigger Sample */
    if (!effect9msg)
      printf("Retrigger Sample Not Implemented Yet.\n");
    effect9msg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xA:  /* Fine Volume Slide up */
    if (!effectAmsg)
      printf("Fine Volume Slide up Not Implemented Yet.\n");
    effectAmsg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xB:  /* Fine Volume Slide down */
    if (!effectBmsg)
      printf("Fine Volume Slide down Not Implemented Yet.\n");
    effectBmsg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xC:  /* Cut Sample */
    if (!effectCmsg)
      printf("Cut Sample Not Implemented Yet.\n");
    effectCmsg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xD:  /* Delay Sample */
    if (!effectDmsg)
      printf("Delay Sample Not Implemented Yet.\n");
    effectDmsg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xE:  /* Delay Pattern */
    if (!effectEmsg)
      printf("Delay Pattern Not Implemented Yet.\n");
    effectEmsg = 1;
    CLEARINCVOLFUN;
    break;

  case 0xF:  /* Invert Loop */
    if (!effectFmsg)
      printf("Invert Loop Not Implemented Yet.\n");
    effectFmsg = 1;
    CLEARINCVOLFUN;
    break;
  }
  return(MP_OK);
}



MPstatus ProcessEffect(void)
{
  static uint8 effect3msg = 0;
  static uint8 effect4msg = 0;
  static uint8 effect5msg = 0;
  static uint8 effect6msg = 0;
  static uint8 effect7msg = 0;
  static uint8 effect9msg = 0;
  static uint8 effectAmsg = 0;

  uint8 e, x, y, z;
  for(channel = 0; channel < mod.number_channels; channel++)
  {
    x = mod.pattern[pattern][division][channel].argx;
    y = mod.pattern[pattern][division][channel].argy;
    e = mod.pattern[pattern][division][channel].effect;

    switch (e)
    {
    case 0x0: case 0x1: case 0x2: case 0x3: case 0x4:
      break;
    default:
      chan_state[channel].curr_samp_inc = chan_state[channel].div_samp_inc;
      break;
    }

    switch (e)
    {
    case 0x0:  /* Arpeggio */
      if (!x && !y)
      /* This will happen a lot...so there is a special case for efficiency  */
      {
        chan_state[channel].curr_samp_inc = chan_state[channel].div_samp_inc;
        CLEARINCVOLFUN;
        break;
      }

      /* As many calculations as possible are done apriori                   */
      chan_state[channel].arpeggio_inc_a = chan_state[channel].div_samp_inc;
      chan_state[channel].arpeggio_inc_b = (chan_state[channel].div_samp_inc 
                                            * semitone_factor[x]) >> 13;
      chan_state[channel].arpeggio_inc_c = (chan_state[channel].div_samp_inc
                                            * semitone_factor[y]) >> 13; 
      /* Calculate the three arpeggio frequencies; stored internally as the  */
      /* increment amounts. We again leave them as 15 b.p. fixed point nums  */

      chan_state[channel].arpeggio_one_third_div_pos = max_div_pos / 3;
      chan_state[channel].arpeggio_two_third_div_pos = (max_div_pos << 1) / 3;
      /* Calculate the Arpeggio position markers                             */
      CLEARVOLFUN;
      SETINCFUN(CalcArpeggioInc);
      break;

    case 0x1:  /* Slide/Portamento up */
      /* Note: max period change per tick is 255.  min tick_buf_size is 39.  */
      /* yielding a possible 6.54 period change per resample iteration.      */
      /* This works out to a possible change in curr_samp_inc of 0.46 (max)  */
      /* on a single iteration.  The max inc change in a tick is approx      */
      /* 22.74.   A 22 b.p. fixed point was chosen (to be conservative)      */
      /* With 32 ticks we could get an offset of approx 736                  */

      if ((!x && !y) || !chan_state[channel].div_samp_inc)
      /* problem, so don't do effect... but keep playing mod */
      {
        chan_state[channel].curr_samp_inc = chan_state[channel].div_samp_inc;
        CLEARINCVOLFUN;
        break;
      }
      z = (x << 4) + y;
      chan_state[channel].slide_period = ((((AMIGA_CLOCK << 8) / out_rate)
                  << 12) / (chan_state[channel].div_samp_inc << 1)) << 15;
      chan_state[channel].slide_delta = (z << 20) / tick_buf_size;

      CLEARVOLFUN;
      SETINCFUN(CalcSlideUpInc);
      break;

    case 0x2:  /* Slide/Portamento down */
      if ((!x && !y) || !chan_state[channel].div_samp_inc)
      {
        chan_state[channel].curr_samp_inc = chan_state[channel].div_samp_inc;
        CLEARINCVOLFUN;
        break;
      }
      z = (x << 4) + y;
      chan_state[channel].slide_period = ((((AMIGA_CLOCK << 8) / out_rate)
                  << 12) / (chan_state[channel].div_samp_inc << 1)) << 15;
      /* should it be curr_samp_inc or div_samp_inc ??? */

      chan_state[channel].slide_delta = (z << 20) / tick_buf_size;

      CLEARVOLFUN;
      SETINCFUN(CalcSlideDownInc);
      break;

    case 0x3:  /* Slide to Note/Tone-portamento */
      if (!effect3msg)
        printf("Slide to Note/Tone-portamento Not Implemented Yet. [%d,%d]\n"
            ,x,y);
      effect3msg = 1;
      if (!x && !y) 
        break; /* Leave the old vol/inc functions (and slide values)  active */
      z = (x << 4) + y;
      
      CLEARINCVOLFUN;
      break;

    case 0x4:  /* Vibrato */
      if (!effect4msg)
        printf("Vibrato Not Implemented Yet.\n");
      effect4msg = 1;
      CLEARINCVOLFUN;
      break;

    case 0x5:  /* Slide to Note plus Volume Slide */
      if (!effect5msg)
        printf("Slide to Note plus Volume Slide Not Implemented Yet.\n");
      effect5msg = 1;
      CLEARINCVOLFUN;
      break;

    case 0x6:  /* Vibrato plus Volume Slide */
      if (!effect6msg)
        printf("Vibrato plus Volume Slide Not Implemented Yet.\n");
      effect6msg = 1;
      CLEARINCVOLFUN;
      break;

    case 0x7:  /* Tremolo */
      if (!effect7msg)
        printf("Tremolo Not Implemented Yet.\n");
      effect7msg = 1;
      CLEARINCVOLFUN;
      break;

    case 0x8:  /* Not Used */
      CLEARINCVOLFUN;
      return(MP_BADEFFECT);

    case 0x9:  /* Set Sample Offset */
      if (!effect9msg)
        printf("Set Sample Offset Not Implemented Yet.\n");
      effect9msg = 1;
      CLEARINCVOLFUN;
      break;

    case 0xA:  /* Volume Slide */
      if (!effectAmsg)
        printf("Volume Slide Not Implemented Yet.\n");
      effectAmsg = 1;
      CLEARINCVOLFUN;
      break;

    case 0xB:  /* Position Jump.  NEEDS MORE TESTING */
      CLEARINCVOLFUN;
      if ((z = (x << 4) + y) < mod.length)
      {
        division = -1;
        /* The - 1 is because after the current division is played the next  */
        /* operation will be to increment the division counter.              */
        song_pos = z;
        pattern = mod.pattern_table[song_pos];
      }
      else
        return(MP_BADJUMPEFFECT);
      break;

    case 0xC:  /* Set Volume */
      CLEARINCVOLFUN;
      if ((z = (x << 4) + y) <= 64)
        chan_state[channel].curr_samp_vol = z;
      else
        chan_state[channel].curr_samp_vol = 64;
      break;

    case 0xD:  /* Pattern Break */
      CLEARINCVOLFUN;
      /* Error checking should probably be implemented for this effect       */
      /* Note that is there are multiple PB effects on different channels    */
      /* we're in big trouble.  Should prob be fixed...                      */
      if ((division = ((x * 10) + y - 1)) >= MOD_NUM_DIVISIONS)
        return(MP_BADJUMPEFFECT);
      /* The - 1 is because after the current division is played the next    */
      /* operation will be to increment the division counter.                */

      if (++song_pos >= mod.length) return(MP_BADJUMPEFFECT); 
      /* if at the end we assume that the pattern table has no more info     */
      pattern = mod.pattern_table[song_pos];
      break;

    case 0xE:  /* E Command */
      ProcessEEffect(x, y);   /* should check return value here */
      break;

    case 0xF:  /* Set Speed */
      if ((z = (x << 4) + y) < 32)
      /* a different MOD spec says <= 32.  Make a command line flag choose.  */
      {
        if (!z) z = 1;
        /* Treat z = 0 as z = 1.  Make a cl flag if we want this behaviour   */
        tpd = z;
      }
      else
        tick_buf_size = (5 * out_rate)/(z << 1);
      max_div_pos = (tpd * tick_buf_size) - 1;
      CLEARINCVOLFUN;
      break;

    default:  /* Can't imagine how this would happen. */
      CLEARINCVOLFUN;
      return(MP_BADEFFECT);
    }
  } 
  return(MP_OK);
}



void SetupChannels(void)
{
  uint16 period;
  uint8 samp_no;

  /* initialize all the channels */
  for (channel = 0; channel < mod.number_channels; channel++)
  {
    period = mod.pattern[pattern][division][channel].period;
    if ((samp_no = mod.pattern[pattern][division][channel].sample_number))
      if ((chan_state[channel].sample_length =
           mod.sample_desc[samp_no - 1].length) > 2)
      {
        chan_state[channel].sample = mod.sample[samp_no - 1];
        chan_state[channel].curr_samp_vol =
          mod.sample_desc[samp_no - 1].volume;

        chan_state[channel].sample_position = 0;
        chan_state[channel].repeat_point =
          mod.sample_desc[samp_no - 1].repeat_point;

        chan_state[channel].repeat_length =
          mod.sample_desc[samp_no - 1].repeat_length;
      }
      else        /* goes with most recent 'if' (ANSI spec)                  */
        chan_state[channel].sample = NULL;
        /* Channel off (having a special case is more efficient) */

    else   /* no sample number */ 
      if (period)
        chan_state[channel].sample_position = 0;
        /* This is what PT does if no sample is given but a period is        */

    /* Beware: The period must be set outside the last if. It must be set    */
    /* regardless of whether there was a new sample. We use old sample rate  */
    /* if period is 0                                                        */

    if (period)
      chan_state[channel].div_samp_inc =
        (((AMIGA_CLOCK << 8) / ( period << 1)) << 7) / out_rate;

    else
      chan_state[channel].div_samp_inc = chan_state[channel].curr_samp_inc;
    /* Lets presume the max value for samp_inc is 15...  This allows a       */
    /* high max pitch (60KHz) and a min resampling rate of 4KHz              */
  }
}



MPstatus PlayDivision(void)
{
  int tick;
  void *t_buf;

  /* Make sure mixer buffer is big enough to handle a whole ticks worth of   */
  /* data.  No need to check return value from Mix then.                     */
  /* Put in spec of mixer that this is guaranteed.  Add a way to query/set   */
  /* the mixer buffer size upon initialization  i.e.it'll need to be dynamic */

  /* We do one tick at a time... allowing for a small mixer buffer size.     */
  for (tick=0; tick < tpd; tick++)
    for (channel = 0; channel < mod.number_channels; channel++)
    {   
      if ((chan_state[channel].sample)) /* && (channel == 1))   channel on */
        ResampleTick(tick);
      else
        ClearTickBuffer();
      t_buf = tick_buf;
      Mix((1 << channel), &t_buf, tick_buf_size);
    }
  return(MP_OK);
}



MPstatus ResampleTick(int tick)
{
  uint32 t, tc;
  uint8* s;
  uint32 l;
  uint8 v, w = 0;
  uint32 (*CCI) (int, int);
  uint32 (*CCV) (int, int);

  s = chan_state[channel].sample;
  v = chan_state[channel].curr_samp_vol;
  l = (chan_state[channel].repeat_length > 2) ? 
       chan_state[channel].repeat_length + chan_state[channel].repeat_point :
       chan_state[channel].sample_length;

  for (t = 0; t < tick_buf_size; t++)
  {
    CCI = chan_state[channel].CalcCurrInc;
    if ((CCI = chan_state[channel].CalcCurrInc))
      chan_state[channel].curr_samp_inc = (*CCI) (tick, t);

    if ((CCV = chan_state[channel].CalcCurrVol))
      v = (*CCV) (tick, t);

    w = (s[ chan_state[channel].sample_position >> 15 ] * v) >> 6;
    tick_buf[t] = w;

    if (((chan_state[channel].sample_position +=
          chan_state[channel].curr_samp_inc) >> 15) >= l)
      if (chan_state[channel].repeat_length > 2)
        chan_state[channel].sample_position =
          chan_state[channel].repeat_point << 15;
      else
      {
        /* Turn off channel for the rest of the ticks if sample completed    */
        /* and we're not supposed to repeat.                                 */
        chan_state[channel].clear_val = w;
        chan_state[channel].sample = NULL;
        break;
      }
  }

  /* The rest of the tick is empty.  Putting the previous value there seems  */
  /* to get rid of the clicks.                                               */
  for (tc = t; tc < tick_buf_size; tc++)
    tick_buf[tc] = w;

  return(MP_OK);
}



/* Presumes that the MOD file has been loaded and the mixer initialized with */
/* the correct number of channels, 8bit resolution, and an arbitrary rate    */
MPstatus PlayMod(void)
{

  /* I should check the return value                                         */
  InitializePlayer();

  for (song_pos = 0; song_pos < mod.length; song_pos++)
  {
    pattern = mod.pattern_table[song_pos];
    for (division = 0; division < MOD_NUM_DIVISIONS; division++)
    {
/*    printf("Pos:%3d   Pat:%3d   TPD:%3d   Div:%3d\r",
              song_pos, pattern, tpd, division); */
      fflush(NULL); 
      SetupChannels();

      /* Do better checking of return value */
      if (ProcessEffect() != MP_OK) break;
      /* probably a bad jump occurred so we skip this division */

      PlayDivision();
    }
  }
  return(MP_OK);
}



