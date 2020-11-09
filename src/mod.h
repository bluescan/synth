/*****************************************************************************/
/* mod.h v0.1     Protracker Module Structure Declarations                   */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Thu Mar 28 14:26:15 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/


/* This file holds the declaration for the data types that will be used to   */
/* access a protracker mod file memory image.  A structure is used to access */
/* data at known offsets while the samples are referred to via an            */
/* array of sample pointers                                                  */

/* We will start with the more common 31 sample (vs. 15) mod format          */
/* It may be possible to use it for the 15 sample format... I'll see later   */

#ifndef mod_h
#define mod_h

/* First we define the error code return valued for any mod player function  */
/* These errorcodes will eventually be put in a super header as they will    */
/* apply to functions concerning the processing of other mod song formats    */
/* such as S3M, 669, etc.                                                    */
typedef enum
{
  MP_OK              = 0,          /* function call was successful           */
  MP_BADFILE         = 1,          /* mod file problem:  non-existent,       */
                                   /* wrong extension, wrong permissions     */
  MP_NOMEM           = 2,          /* system memory request failed           */
  MP_BADARGS         = 4,          /* bad or wrong num of command line args  */
  MP_BADTABLE        = 8,          /* corrupt pattern table                  */
  MP_BADDESC         = 16,         /* description string not recognized      */
  MP_UNEXPECTED_EOF  = 32,
  MP_UNAVAIL_DSP     = 64,         /* dsp device not found or locked         */
  MP_DSP_ERROR       = 128,
  MP_DSP_BADARG      = 256,
  MP_BADEFFECT       = 512,        /* Invalid effect or effect arguments     */
  MP_BADJUMPEFFECT   = 1024        /* Invalid jump effect parameters         */

  /* others to be added as we go */
} MPstatus;

/* These are some typedefs.  They will help if this code ever needs to be    */
/* ported to machines using a different number of bytes for the specified    */
/* types.  This stuff should eventually go into the "superheader".           */
typedef unsigned short int uint16;
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned int uint32;
typedef signed short int int16;


#define LOAD_BLK_SIZE 1024    /* A good size for reading file chunks         */
                              /* This value should be checked for efficiency */

/* These definitions are just for protracker mods                            */
#define MOD_PATTERN_TABLE_SIZE 128
#define MOD_DESC_SIZE 4
#define MOD_SAMPLE_NAME_SIZE 22
#define MOD_TITLE_NAME_SIZE 20
#define MOD_NUM_SAMPLES 31         /* ones with 15 have 0 length in the rest */
#define MOD_CHANNELS 4
#define MOD_MAX_CHANNELS 8         /* most mods use only 4 but max is 8      */
#define MOD_PATTERNS 64            /* although 64 is the most common value   */
#define MOD_MAX_PATTERNS  128      /* protracker may use more                */
#define MOD_NUM_DIVISIONS 64
#define MOD_EXTENSION "mod"


struct mod_samp_desc
{
  char name[MOD_SAMPLE_NAME_SIZE];
  uint32 length;                   /* length holds the size in bytes, !words */
  uint8 finetune;                  /* use unsigned since straight char       */
                                   /* could be either. LS nibble used        */
  uint8 volume;                    /* 0..64 inclusive!                       */
  uint32 repeat_point;             /* repeat_point and repeat_length will    */
  uint32 repeat_length;            /* hold byte values, not number of words  */
};

struct mod_channel_info            
{
  uint8 sample_number;
  uint16 period;                   /* Only 12 LS bits used for both          */
  uint8 effect;
  uint8 argx;
  uint8 argy;
};

/* The mod_data structure can be used to access any of the mod file's fields */
/* except for the sample data                                                */
/* The patterns array (although a slight waste of space... the speed is      */
/* necessary) allows access to any piece of channel data in a pattern        */
/* Usage would be: pattern[pattern_no 0..MOD_MAX_PATTERNS-1]                 */
/*                        [division_no 0..MOD_NUM_DIVISIONS-1]               */
/*                        [channel no 0..MOD_MAX_CHANNELS-1].period etc.     */
/* The contents will be copied from the mod file to the appropriate place    */
/* The sample array is just an array of pointers to the start of each        */
/* sample.  Usage would be: mod.sample[sample_no 0..MOD_NUM_SAMPLES]         */
/*                                [index_into_sample 0..sample_desc.length]  */
struct mod_data
{
  char title[MOD_TITLE_NAME_SIZE];
  struct mod_samp_desc sample_desc[MOD_NUM_SAMPLES];
  uint8 length;                    /* Song size in # of patterns (1..128)    */
  uint8 ignore;
  uint8 pattern_table[MOD_PATTERN_TABLE_SIZE];
  uint8 highest_pattern;
  char description[MOD_DESC_SIZE+1]; /* Is this always present ??            */
  uint8 number_channels;
  struct mod_channel_info pattern[MOD_MAX_PATTERNS]
                                 [MOD_NUM_DIVISIONS]
                                 [MOD_MAX_CHANNELS];
  uint8*  sample[MOD_NUM_SAMPLES];
};

/* Note:  No initialization necessary since it's declared globally           */
extern struct mod_data mod;
  

#endif
