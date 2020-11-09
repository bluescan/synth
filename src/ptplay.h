/*****************************************************************************/
/* ptplay.h v0.1        Protracker (MOD) Play Declarations                   */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Thu Apr 18 04:13:11 PDT 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/

#ifndef ptplay_h
#define ptplay_h

#include "mod.h"

/* This is the maximum number of samples that may be needed to process one   */
/* tick.  i.e. assume highest freq (44100Hz), smallest bpm (33 or 32         */
/* depending on which MOD specification you read... I like the 32)           */
/* seconds per tick = 2.5/32 so max buf size is 3445.3125                    */
#define MAX_TICK_BUFFER_SIZE 3446
#define AMIGA_CLOCK 7093789         /* This is the PAL (!NTSC) clock speed  */
/* #define AMIGA_CLOCK 7159091  NTSC clock speed */
/* #define AMIGA_CLOCK 10541918 */
/* #define AMIGA_CLOCK 5270959 */

#define MOD_SLIDE_MIN_PER 113
#define MOD_SLIDE_MAX_INC 0x3D4E0000
/* represents period 113 and based on the above AMIGA_CLOCK rate.  It's not  */
/* currently used.                                                           */

#define MOD_SLIDE_MAX_PER 856
#define MOD_SLIDE_MIN_INC 0x08178000
/* representd period 856 and based on the above AMIGA_CLOCK rate.  It's not  */
/* currently used.                                                           */

MPstatus PlayMod(void);

#endif
