/*****************************************************************************/
/* main.c v0.1                Main Test Driver                               */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Fri Mar 29 12:17:44 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#include <stdio.h>
#include "mod.h"
#include "buildinfo.h"
#include "loadmod.h"
#include "exiterror.h"
#include "mixer.h"
#include "ptplay.h"

/* extern struct mod_data mod;  has been put in mod.h */

/* This getline fn was taken from K&R.  */
int getline(char s[], int lim)
{
  int c = 0, i;
  for (i=0; i<lim-1 && (c=getchar())!=EOF && c!='\n'; ++i)
    s[i] = c;
  if (c == '\n') {
    s[i] = c;
    ++i;
  }
  s[i] = '\0';
  return i;
}


int main(int argc, char** argv)
{
  MPstatus status;
  char modname[256];
  FILE* mod_fd;
  int n;
  int rate, res;
  char line[80];

  if ((status = ParseCommandLine(argc, argv, modname)) != MP_OK)
    ExitError(status);

  if ((status = OpenSong(modname, &mod_fd)) != MP_OK)
    ExitError(status);

  if ((status = LoadMod(mod_fd)) != MP_OK)
    ExitError(status);

  printf("\nMODify v0.1   Protracker, Noisetracker, Soundtracker Module Player\n");
  printf("Copyright 1996 Tristan Grimmer.  All rights reserved.\n\n");

  printf("Built on %s by %s, at %s\n\n", HOST, USER, DATE);

  printf("MOD TITLE: %s\n", mod.title);
  printf("-----------------------------------------------------------------------------\n");
  printf("| SAMP# |      INSTR NAME        |  LEN  | FTUNE | DVOL | REP_PNT | REP_LEN |\n");
  printf("-----------------------------------------------------------------------------\n");
  for(n=0; n<MOD_NUM_SAMPLES;n++)
  {
    if (mod.sample_desc[n].length)
    {
      printf("| %2d    | %22.22s ", n+1, mod.sample_desc[n].name);
      printf("| %5u |  %2u   | %3u  |  %5u  |  %5u  |\n",
        mod.sample_desc[n].length, mod.sample_desc[n].finetune,
        mod.sample_desc[n].volume, mod.sample_desc[n].repeat_point,
        mod.sample_desc[n].repeat_length);
    }
  }
  printf("-----------------------------------------------------------------------------\n");
  printf("SONG LENGTH: %u  ", mod.length);
  printf("IGNORE: %u  ", mod.ignore);
/*  printf("SONG PATTERN TABLE: ");
  for(n=0; n<MOD_PATTERN_TABLE_SIZE; n++) printf("%u, ",mod.pattern_table[n]);
  printf("\n");*/

  printf("DESCRIP: %s\n", mod.description);

  
 
/*  for(l=0; l <= mod.highest_pattern; l++)
    for(n=0; n<MOD_NUM_DIVISIONS; n++)
       for(m=0; m<mod.number_channels; m++)
         printf("PATTERN: %d  DIVISION: %d  CHANNEL: %d  sample_no: %u  period: %u  effect: %x  argx: %x  argy: %x\n",l, n, m+1,
                 mod.pattern[l][n][m].sample_number,
                 mod.pattern[l][n][m].period,
                 mod.pattern[l][n][m].effect, mod.pattern[l][n][m].argx, mod.pattern[l][n][m].argy); */
 
  printf("CHANS: %u\n", mod.number_channels);
/*  printf("HIGHEST PATTERN: %u\n", mod.highest_pattern); */

/*  printf("PLAYING SAMPLES\n");

  if ((status = PlaySamples()) != MP_OK) ExitError(status);  */

/*  rate = 11025; */
  rate = 22050;
/*  rate = 8000; */
/*  rate = 14000; */
  printf("Enter Frequency[22050]: ");
  getline(line, sizeof(line));
  sscanf(line, "%d", &rate);
  res = 8;
/*  if ((status = MixerInitialize(rate, res, CHANNEL1 | CHANNEL4, CHANNEL2 | CHANNEL3)) */
  if ((status = MixerInitialize(rate, res, CHANNEL1 | CHANNEL4 | CHANNEL2 | CHANNEL3, 0))
             != MP_OK) ExitError(status);

  printf("Using Rate: %d\n", MixerGetRate());

  PlayMod();

  printf("\nExiting...\n");
  return 0;
}
