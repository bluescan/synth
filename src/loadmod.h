/*****************************************************************************/
/*                                                                           */
/* loadmod.h v0.1           Load Module Declarations                         */
/*                                                                           */
/* Created by: Tristan Grimmer                                               */
/* Email: tgrimmer@unixg.ubc.ca                                              */
/* Creation Date: Fri Mar 29 03:35:36 PST 1996                               */
/* Last Modified:                                                            */
/* Comments:                                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef loadmod_h
#define loadmod_h

#include <stdio.h>
#include "mod.h"


MPstatus ParseCommandLine(int argc, char** argv, char* ret_filename);
MPstatus OpenSong(const char* filename, FILE** ret_fd);
/* CloseSong still needs to be done...Don't forget to free all the memory    */
/* malloced by LoadModAllSamples as well as close the file                   */
/* Note: The above two fn's should eventually be part of a different         */
/* subsystem as they are not specific to PT mod files                        */


MPstatus LoadMod(FILE* fd);

#endif



