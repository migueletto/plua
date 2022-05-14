#include "p.h"
#include "midi.h"

/*
You can play a tune by passing in a Level 0 Standard MIDI
File (SMF) through the SndPlaySmf function.
Although you an use a Level 0 Standard MIDI File to control simple
sound generation, this doesn't imply broad support for MIDI
messages: Only key down, key up, and tempo change messages are
recognized.
*/

/*
static void MidiCompletion(void* channel, UInt32 userData)
{
}

static Boolean MidiBlocking(void* channel, UInt32 userData, Int32 time)
{
  return true;
}
*/

Err MidiPlay(char *name, Int32 amp)
{
  SndSmfOptionsType smfop;
  //SndSmfChanRangeType smfrange;
  //SndSmfCallbacksType smfcb;
  //SndCallbackInfoType smf_completion, smf_blocking, smf_reserved;
  FILE *f;
  Err err;

  if ((f = fopen(name, "r")) == NULL)
    return -1;

  if (f->type != FILE_PDB && f->type != FILE_RESOURCE) {
    fclose(f);
    errno = EINVAL;
    return -1;
  }

  smfop.dwStartMilliSec = 0;
  smfop.dwEndMilliSec = sndSmfPlayAllMilliSec;
  smfop.amplitude = amp; // 0-64
  smfop.interruptible = true;
  smfop.reserved1 = 0;
  smfop.reserved = 0;

/*
  smfrange.bFirstChan = 0;
  smfrange.bLastChan = 0;

  smf_completion.funcP = NULL;
  smf_completion.dwUserData = 0;
  smf_blocking.funcP = NULL;
  smf_blocking.dwUserData = 0;
  smf_reserved.funcP = NULL;
  smf_reserved.dwUserData = 0;

  smfcb.completion = smf_completion;
  smfcb.blocking = smf_blocking;
  smfcb.reserved = smf_reserved;
*/

  err = SndPlaySmf(NULL, sndSmfCmdPlay, (UInt8 *)f->record, &smfop, NULL, NULL, true);
  fclose(f);

  errno = mapPalmOsErr(err);
  return err ? -1 : 0;
}
