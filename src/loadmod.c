/*****************************************************************************/
/* loadmod.c v0.1             Load Module Data                               */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Fri Mar 29 02:04:09 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#include <string.h>
#include "loadmod.h"
#include "mod.h"
#include <stdio.h>    /* get rid of this one later */
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#ifdef PLAT_LINUX
#include <unistd.h>
#endif

/* Local prototypes                                                          */
static void LowerString(char* st);
static MPstatus LoadModPattern(FILE* fd, int pat_no, int num_channels);
static MPstatus LoadModSampleDescription(FILE* fd, int sample_no);
static MPstatus LoadModTitle(FILE* fd);
static MPstatus LoadModAllSampleDescriptions(FILE* fd);
static MPstatus LoadModLength(FILE* fd);
static MPstatus LoadModIgnore(FILE* fd);
static MPstatus LoadModPatternTable(FILE* fd);
static MPstatus LoadModDescription(FILE* fd);
static MPstatus SetModHighestPattern(void);
static MPstatus SetModNumberChannels(void);
static MPstatus LoadModAllPatterns(FILE* fd);
static MPstatus LoadModSample(FILE* fd, int8* mem_ptr, uint32 samp_size);
static MPstatus LoadModAllSamples(FILE* fd);

/* Mod definition.  This struct will be declared extern in other files.      */
/* This should be the only global variable (the speed is necessary)          */
struct mod_data mod;


/* Converts a string to all lower case.  Presumes ASCII or other             */
/* "consecutive" letter representation.  Presumes that the string is not     */
/* empty.  i.e. first chat can't be '\0'                                     */
void LowerString(char* st)
{
  do
    if ((*st >= 'A') && (*st <= 'Z'))
      *st = *st - 'A' + 'a';
  while (*++st);
}


/* Extracts filename.  Returns the name in m_name                            */
MPstatus ParseCommandLine(int argc, char** argv, char* m_name)
{
  char buf[256];

  if (argc != 2) 
    return(MP_BADARGS);
  strcpy(buf,argv[1]);
  LowerString(buf);

  /* Check extension.  First three or last three letters must be "mod"       */
  /* Eventually there will be a separate function                            */
  if (strncmp(MOD_EXTENSION, buf, 3) &&
     strncmp(MOD_EXTENSION, buf + strlen(buf) - strlen(MOD_EXTENSION), 3))
    return(MP_BADFILE);

  /* return the filename                                                     */
  strcpy(m_name, argv[1]);
  return(MP_OK);
}


/* The const means the array will not be altered.  This function will        */
/* eventually be moved as it can be used for any song format.  The file      */
/* descriptor is returned in fd                                              */
MPstatus OpenSong(const char* songname, FILE** fd)
{
	if ((*fd = fopen(songname, "rb")) == NULL)
  // if ((*fd = open(songname, O_RDONLY, 0)) == -1)
    return(MP_BADFILE);
  return(MP_OK);
}



/* Modifies global "mod" variable                                            */
/* Reads the MOD file's title.  The file position is assumed to be correct.  */
/* i.e. the order in which the LoadMod.... functions are called is important */
MPstatus LoadModTitle(FILE* mod_fd)
{
  if (fread(mod.title, 1, MOD_TITLE_NAME_SIZE, mod_fd) != MOD_TITLE_NAME_SIZE)
    return(MP_BADFILE);
  return(MP_OK);
}



/* Modifies global "mod" variable                                            */
MPstatus LoadModSampleDescription(FILE* mod_fd, int samp_number)
{
  uint8 buf[2];
  uint16 word;

  if (fread(mod.sample_desc[samp_number].name, 1, MOD_SAMPLE_NAME_SIZE, mod_fd) != MOD_SAMPLE_NAME_SIZE)
    return(MP_BADFILE);

  /* NOTE: we can't directly read to a word variable because of the          */
  /* endian-ness problem                                                     */
  if (fread(buf, 1, 2, mod_fd) != 2)
    return(MP_BADFILE);
 
  word = (((uint16) buf[0]) << 8) + (uint16) buf[1];

  /* Convert from length in words to length in bytes                         */
  mod.sample_desc[samp_number].length = ((uint32) word) << 1;

  if (fread(&mod.sample_desc[samp_number].finetune, 1, 1, mod_fd) != 1)
    return(MP_BADFILE);
  /* Only LS nibble used.  This clears the upper 4 bits just in case.        */
  mod.sample_desc[samp_number].finetune &= 0x0F;

  if (fread(&mod.sample_desc[samp_number].volume, 1, 1, mod_fd) != 1)
    return(MP_BADFILE);

  if (fread(buf, 1, 2, mod_fd) != 2)
    return(MP_BADFILE);
  word = (((uint16) buf[0]) << 8) + (uint16) buf[1];
  mod.sample_desc[samp_number].repeat_point = ((uint32) word) << 1;


  if (fread(buf, 1, 2, mod_fd) != 2)
    return(MP_BADFILE);
  word = (((uint16) buf[0]) << 8) + (uint16) buf[1];
  mod.sample_desc[samp_number].repeat_length = ((uint32) word) << 1;

  return(MP_OK);
}



MPstatus LoadModAllSampleDescriptions(FILE* mod_fd)
{
  int n;
  MPstatus status;
  for (n=0; n < MOD_NUM_SAMPLES; n++)
    if ((status = LoadModSampleDescription(mod_fd, n)) != MP_OK)
      return(status); 
  return(MP_OK);
}


  
MPstatus LoadModLength(FILE* mod_fd)
{
  if (fread(&mod.length, 1, 1, mod_fd) != 1)
    return(MP_BADFILE);
  return(MP_OK);
}



MPstatus LoadModIgnore(FILE* mod_fd)
{
  if (fread(&mod.ignore, 1, 1, mod_fd) != 1)
    return(MP_BADFILE);
  return(MP_OK);
}



MPstatus LoadModPatternTable(FILE* mod_fd)
{
  int numread;
  numread = fread(mod.pattern_table, 1, MOD_PATTERN_TABLE_SIZE, mod_fd);
  if (numread != MOD_PATTERN_TABLE_SIZE)
    return(MP_BADFILE);
  return(MP_OK);
}



MPstatus LoadModDescription(FILE* mod_fd)
{
  if (fread(mod.description, 1, MOD_DESC_SIZE, mod_fd) != MOD_DESC_SIZE)
    return(MP_BADFILE);
  mod.description[MOD_DESC_SIZE] = '\0';
  return(MP_OK);
} 



MPstatus LoadModPattern(FILE* mod_fd, int pat_no, int num_channels)
{
  uint8 cdata[4];
  int channel, division;
  uint16 temp_word = 0;

  for(division = 0; division < MOD_NUM_DIVISIONS; division++)
    for(channel = 0; channel < num_channels; channel++)
    {
      if (fread(cdata, 1, 4, mod_fd) != 4)
        return(MP_BADFILE);

      /* Note: We are guaranteed logical shift with unsigned quantities */
      mod.pattern[pat_no][division][channel].sample_number = 
        (cdata[0] & 0xF0) + (cdata[2] >> 4);

      /* Remember what is going on!  The PC is little-endian while the  */
      /* Amiga is big-endian.  The cast sticks the LSB in the first     */
      /* byte of the word.  The shift will relocate it to the second.   */
      /* Note that the casts are not strictly necessary since doing a   */
      /* logical shift automatically performs an integral promotion.    */
      temp_word = ((uint16) (cdata[0] & 0x0F)) << 8;  
      mod.pattern[pat_no][division][channel].period = 
        temp_word + ((uint16) cdata[1]);

      temp_word = ((uint16) (cdata[2] & 0x0F)) << 8;
      temp_word += ((uint16) cdata[3]);

      mod.pattern[pat_no][division][channel].effect = (uint8) (temp_word >> 8);
      mod.pattern[pat_no][division][channel].argx = (uint8) ((temp_word & 0x00F0) >> 4);
      mod.pattern[pat_no][division][channel].argy = (uint8) (temp_word & 0x000F);
      /* Note: The ANSI standard guaranteed left bits will be ignored   */
      /* during the integer conversions.  This could probably be done   */
      /* faster... but this stuff isn't timing critical.                */
 
    };
  return(MP_OK);
}



/* Sets mod.highest_pattern to the highest pattern found in the pattern */
/* table.  Requires that both the                                       */
/* pattern table AND the description have been loaded (as we can check  */
/* if the number of patterns is allowed to go over 63                   */ 
MPstatus SetModHighestPattern(void)
{
  int n;
  uint8 big = 0, curr;
  for(n=0; n<MOD_PATTERN_TABLE_SIZE; n++)
    big = ((curr = mod.pattern_table[n]) > big) ? curr : big;
  if (big >= MOD_MAX_PATTERNS)
    return(MP_BADTABLE);

  /* if there is reference to patterns above 63 and we are not a        */
  /* protracker mod then there's trouble                                */
  if ((big >= MOD_PATTERNS) && (strcmp("M!K!", mod.description)))
    return(MP_BADTABLE);

  /* set the number of patterns in the mod structure                    */
  mod.highest_pattern = big;
  return(MP_OK);
}



/* Sets the number of channels by looking at the description (which     */
/* must have been previously loaded)                                    */
MPstatus SetModNumberChannels(void)
{
  char* descriptions[] = { "M.K.", "M!K!", "FLT4", 
                           "FLT8", "6CHN", "8CHN" };
  int n, desc = -1;

  for (n=0; n<6; n++)
    if (!strcmp(descriptions[n],mod.description))
    {
      desc = n;
      break;
    }

  if (desc == -1)
    return(MP_BADDESC);

  switch (desc)
  {
    case 0: case 1: case 2: 
      mod.number_channels = 4;
      break;
    case 3: case 5:
      mod.number_channels = 8;
      break;
    case 4:
      mod.number_channels = 6;
      break;
    default:
      return(MP_BADDESC);
  }  
  return(MP_OK);
}



/* Calls LoadModPattern correct number of times with correct parameters */
/* Requires that both SetModHighestPattern and SetModNumberChannels     */
/* were called previously                                               */
MPstatus LoadModAllPatterns(FILE* mod_fd)
{
  MPstatus status;
  int pattern;
  for(pattern = 0; pattern <= mod.highest_pattern; pattern++)
    if ((status = LoadModPattern(mod_fd, pattern, mod.number_channels))
         != MP_OK)
      return(status);
  return(MP_OK);
}



/* Presumes correct amount of space is already malloced                 */
MPstatus LoadModSample(FILE* mod_fd, int8* mem_ptr, uint32 size)
{
  int amt_to_read, num_read, n;

  while (size) 
  {
    amt_to_read = (size < LOAD_BLK_SIZE) ? ((int) size) : LOAD_BLK_SIZE;

    if ((num_read = fread(mem_ptr, 1, amt_to_read, mod_fd)) == -1)
      return(MP_BADFILE);
    if (num_read == 0)
      return(MP_UNEXPECTED_EOF);

    /* Convert the signed byte samples to unsigned for the sound driver */
    for(n=0; n<num_read; n++)
      mem_ptr[n] ^= 0x80;

    size -= num_read;      /* It's OK, num_read will be positive        */
    mem_ptr += num_read;
  }
  return(MP_OK);
}



/* Note: Any NULL pointers in the sample array mean that sample doesn't */
/* exist                                                                */
MPstatus LoadModAllSamples(FILE* mod_fd)
{
  int8 * mem_ptr;
  MPstatus status;
  int n;

  /* OK, this loop is tricky.  mem_ptr must be reset to NULL each time  */
  /* just in case there is an intermediary sample with zero length      */
  /* This avoids assuming that malloc(0) always returns NULL            */

  for(n=0; n<MOD_NUM_SAMPLES; n++)
  {
    mem_ptr = NULL;

    /* Note, we rely on short circuit evaluation here so that the       */
    /* malloc is not called unnecessarily.  S/C eval is an ANSI feature */

    if ((mod.sample_desc[n].length > 0) && 
       (!(mem_ptr = (int8 *) malloc(mod.sample_desc[n].length))))
      return(MP_NOMEM);
    else
      mod.sample[n]=mem_ptr;

    if (mem_ptr)
    {
      /* printf("\nCallLMS with length %d \n",mod.sample_desc[n].length);    */
      status = LoadModSample(mod_fd, mem_ptr, mod.sample_desc[n].length);
      if (status != MP_OK) return(status);
    }
  }
  return(MP_OK);
}



/* Given a valid mod file descriptor this function will completely load */
/* the mod file data into both the mod structure and the samples array  */
/* It is under debate as to whether a return should be executed upon    */
/* the first noticed error or to logically "or" them all together      */
MPstatus LoadMod(FILE* mod_fd)
{  
  MPstatus status;

  status = LoadModTitle(mod_fd) |
           LoadModAllSampleDescriptions(mod_fd) |
           LoadModLength(mod_fd) |
           LoadModIgnore(mod_fd) |
           LoadModPatternTable(mod_fd) |
           LoadModDescription(mod_fd) |
           SetModHighestPattern() |
           SetModNumberChannels() |
           LoadModAllPatterns(mod_fd) |
           LoadModAllSamples(mod_fd); 

  return(status);  
}






