/*****************************************************************************/
/* exiterror.c v0.1           Error Exit Status                              */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date:  Fri Mar 29 12:40:51 PST 1996                              */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include "exiterror.h"

void ExitError(int error_code)
{
  /* this fn should also check the error */
  /* and display an appropriate message */
  fprintf(stderr,"ErrorCode: %d\n", error_code);
  fprintf(stderr,"Usage: MODify filename.mod\n");
  exit(error_code);
}
