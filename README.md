# Synth
An Amiga mod player written in C using Port Audio.

### To Do
The current outstanding items for synth:

* Should LoadMod return the bitwise OR of all the Load function calls,
  or should it fail after the first failure??  file: loadmod.c/h

* I have a way to open a mod file, but no associated close method.
  When this is implemented it should free all the malloced space used
  by the mod (or should that be a different fn.... allowing the mod
  file to be closed but still having the mod struct in tact??)
  file: loadmod.c/h
  I see nothing wrong with leaving the file open when the mod struct
  is being used... the close routine could then do all the freeing.

* Mixer testing.  Mono modes and 16bit modes not tested thoroughly.
  file: mixer.c/h.  Actually, it is now verified that mono mode doesn't work.

* ExitError function not complete.  It should tell user what error
  code it encountered...not just the value.
  file: exiterror.c/h

* Some of the mixer functions have not been implemented yet.  See
  the comments and prototypes in the file mixer.c

* Makefile does not automatically generate the .depend dependancy
  file if it does not exist.

* file: mixer.c, fn: Mix.  I don't check the return value of the
  Flush call.

* file: mixer.c.  The fn PlaySamples should probably now be scrapped or
  modified so that it uses the mixer.

* file: ptplay.c, fn: PlayMod.  I don't check the return value from
  InitializePlayer... I should.

* file: ptplay.c/h.  There is a way to initialize the player and get
  memory, but no associated free method.

* file: hmmm?.  The song contains some pops and clicks that should not be
  there.  probably a prob with the mixer... but possibly the player (ptplay)
  also.

* When I clear the rest of a tick because the sample has ended I repeat
  the last value so there is no pop/click.  However, when I play an "empty"
  sample I mix in zeros.  Most MODS should be OK with this... but will
  some "pop" by assuming that sending an empty sample works differently.
  i.e. by repeating the last value on that channel...??? file: ptplay.c
  fn: ResampleTick

* The specification I have for MODs says that the sample should be played
  completely through before jumping back to the repeat point and then
  playing for the repeat length.  This is NOT what I am implementing...However,
  It does make sense.  I start repeating when the position reaches the 
  repeat point plus the length.  Verify my behaviour is correct....ask at
  a newsgroup??? is the spec I was given wrong??

* I have no clue whether my sample looping is working properly or not...
  I'll test it when I've implemented enought effects to tell.

* After implementinf effect $F the song getdown.mod doesn't seem to be 
  timed correctly.  It uses effect $F (speed) tonnes.

* Effect: $D Pattern Break, File: ptplay.c.  No error checking. Is argument
  within legal range?  Make sure we aren't in the last pattern as well.
  ...undesireable results when more than one 
  channel contains effect $D (actually... is my behaviour reasonable??)


### Notes

  Include files get included in any file that makes reference
  to any of the data structures/types in the include file.  For example,
  in mixer.h we include mod.h because there is reference to MPstatus for 
  example.  We also include mod.h in mixer.c (although not strictly required)
  because it also makes references to things like MPstatus.  The #ifndef's 
  take care of everything properly.

### Code Conventions

  functions: FirstLetterCapitialized
  variables (global and local):  alllowercasenospaces
  defines (constants): CAPS_USING_UNDERSCORES
  errorcodes (enums): WORD_MOREWORDSWITHOUTUNDERSCORE
  defines (macros): CAPSNOUNDERSCORES
  C source files: filename.c
    (all lower case, no underscores, spaces, or special chars (to remain compatible
    with other OS's.  First 8 chars should be unique i.e. no other .c file shares them.)
  C header files: filename.h (same as above except ends in .h)
