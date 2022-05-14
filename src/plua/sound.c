#include "p.h"
#include "main.h"
#include "events.h"
#include "endian.h"
#include "sound.h"

#define SOUND_FEATURE	1
#define SOUND_SLOTS	3
#define SOUND_BUFFER	4096

typedef struct {
  UInt16 slot;
  FILE *f;     
  SndStreamRef streamRef;
  Boolean finished;
} SoundCallbackData;


//  'R', 'I', 'F', 'F',     0: 'RIFF' magic
//    0,   0,   0,   0,     4: 'RIFF' length
//  'W', 'A', 'V', 'E',     8: 'RIFF' type
//  'f', 'm', 't', ' ',    12: 'fmt ' chunk-type
//   18,   0,   0,   0,    16: 'fmt ' chunk-length
//    1,   0,              20: WAVE_FORMAT_PCM
//    1,   0,              22: Channels
//    0,   0,   0,   0,    24: Samples per second
//    0,   0,   0,   0,    28: Bytes per second
//    1,   0,              32: Aligned bytes per sample group
//    8,   0,              34: Bits per sample
//    0,   0		   36: ??? (opcional)

typedef struct {
  UInt32 riff;
  UInt32 riffLen;
  UInt32 wave;
  UInt32 fmt;
  UInt32 fmtLen;
  UInt16 format, channels;
  UInt32 samplesPerSec;
  UInt32 bytesPerSec;
  UInt16 bytesPerSample, bitsPerSample;
} WaveHeader;

typedef struct {
  UInt32 id;
  UInt32 len;
} WaveSection;


static Int32 SoundRead(void *buf, UInt32 size, FILE *f)
{
  Err err;
  Int32 m;
  UInt32 r = 0;

  if (!f || !buf || !size)
    return r;

  switch (f->type) {
    case FILE_STREAM:
      r = FileRead(f->f, buf, 1, size, &err);
      break;
    case FILE_VFS:
      err = VFSFileRead(f->fref, size, buf, &m);
      r = m < 0 ? 0 : m;
      break;
    case FILE_PDB:
    case FILE_RESOURCE:
      if ((f->recordPos + size) > f->recordSize)
        size = f->recordSize - f->recordPos;
      if (size) {
        MemMove(buf, ((UInt8 *)f->record) + f->recordPos, size);
        f->recordPos += size;
      }
      r = size;
  }

  return r;
}

static Err SoundVarCallbackFunc(void* data, SndStreamRef stream, void *buffer,
                                UInt32 *bufferSize)
{
  UInt32 n;
  SoundCallbackData *cb;
  EventType event;

  cb = (SoundCallbackData *)data;

  if (cb->finished) {
    *bufferSize = 0;
    return 0;
  }

  n = SoundRead(buffer, *bufferSize, cb->f);

  if (n != *bufferSize) {
    cb->finished = true;
    MemSet(&event, sizeof(EventType), 0);
    event.eType = sampleStopEvent;
    event.screenX = cb->slot; // usa campo screenX para armazenar slot
    EvtAddUniqueEventToQueue(&event, 0, true);
  }

  *bufferSize = n;
  return 0;
}

Int32 SoundPlay(UInt16 slot, char *name, Int32 amp)
{
  SoundCallbackData *cb;
  SndStreamWidth sampleWidth;
  SndSampleType sampleType;
  UInt32 sampleRate, value, frameWidth;
  Int32 duration;
  WaveHeader wav;
  WaveSection section;
  UInt16 pad;
  Err err;

  if (slot >= SOUND_SLOTS ||
      FtrGet(sysFileCSoundMgr, sndFtrIDVersion, &value) != 0) {
    errno = EINVAL;
    return -1;
  }

  SoundStop(slot, true);

  if ((cb = malloc(sizeof(SoundCallbackData))) == NULL)
    return -1;

  if ((cb->f = fopen(name, "r")) == NULL) {
    err = errno;
    free(cb);
    errno = err;
    return -1;
  }

  if (cb->f->type != FILE_VFS &&
      cb->f->type != FILE_STREAM &&
      cb->f->type != FILE_PDB &&
      cb->f->type != FILE_RESOURCE) {
    fclose(cb->f);
    free(cb);
    errno = EINVAL;
    return -1;
  }

  if (fread(&wav, 1, 36, cb->f) != 36 ||
      wav.riff != 'RIFF' || wav.wave != 'WAVE' || wav.fmt != 'fmt ') {
    fclose(cb->f);
    free(cb);
    errno = EINVAL;
    return -1;
  }
  wav.riffLen = ByteSwap32(wav.riffLen);
  wav.fmtLen = ByteSwap32(wav.fmtLen);
  wav.channels = ByteSwap16(wav.channels);
  wav.samplesPerSec = ByteSwap32(wav.samplesPerSec);
  wav.bitsPerSample = ByteSwap16(wav.bitsPerSample);

  if ((wav.fmtLen != 16 && wav.fmtLen != 18) ||
      (wav.channels != 1 && wav.channels != 2) ||
      (wav.bitsPerSample != 8 && wav.bitsPerSample != 16) ||
      wav.format != 0x0100) {
    fclose(cb->f);
    free(cb);
    errno = EINVAL;
    return -1;
  }

  if (wav.fmtLen == 18)
    fread(&pad, 1, sizeof(UInt16), cb->f);

  for (;;) {
    if (fread(&section, 1, 8, cb->f) != 8) {
      fclose(cb->f);
      free(cb);
      errno = EINVAL;
      return -1;
    }
    section.len = ByteSwap32(section.len);
    if (section.id == 'data')
      break;

    // ignora secoes desconhecidas

    if (fseek(cb->f, section.len, SEEK_CUR) != 0) {
      fclose(cb->f);
      free(cb);
      errno = EINVAL;
      return -1;
    }
  }

  sampleRate = wav.samplesPerSec;
  sampleType = wav.bitsPerSample == 8 ? sndUInt8 : sndInt16Little;
  sampleWidth = wav.channels == 1 ? sndMono : sndStereo;

  cb->slot = slot;
  cb->finished = false;
  frameWidth = sampleType == sndUInt8 ? wav.channels : wav.channels * 2;

  if ((err = SndStreamCreateExtended(&(cb->streamRef), sndOutput, sndFormatPCM,
                sampleRate, sampleType, sampleWidth, SoundVarCallbackFunc,
                cb, 0, false)) != 0) {
    fclose(cb->f);
    free(cb);
    errno = mapPalmOsErr(err);
    return -1;
  }

  if (amp > 0) {
    if (amp <= 32)
      amp *= 32; // 1-32 -> 32-1024
    else
      amp *= 64; // 33-64 -> 2112-4096
  }

  SndStreamSetVolume(cb->streamRef, amp);

  if ((err = SndStreamStart(cb->streamRef)) != 0) {
    SndStreamDelete(cb->streamRef);
    fclose(cb->f);
    free(cb);
    errno = mapPalmOsErr(err);
    return -1;
  }

  // duracao em 1/100 segundos
  duration = ((section.len / frameWidth) * 100) / wav.samplesPerSec;

  FtrSet(GetCreator(), SOUND_FEATURE+slot, (UInt32)cb);

  return duration;
}

void SoundStop(UInt16 slot, Boolean close)
{
  UInt32 value;
  SoundCallbackData *cb;
  
  if (slot < SOUND_SLOTS &&
      FtrGet(sysFileCSoundMgr, sndFtrIDVersion, &value) == 0) {
    if (FtrGet(GetCreator(), SOUND_FEATURE+slot, (UInt32 *)&cb) == 0) {
      FtrUnregister(GetCreator(), SOUND_FEATURE+slot);

      if (cb) {
        cb->finished = true;
        if (close) {
          SndStreamStop(cb->streamRef);
          fclose(cb->f);
          SndStreamDelete(cb->streamRef);
        }
        free(cb);
      }
    }
  }
}

void SoundStopAll(Boolean close)
{
  Int16 slot;

  for (slot = 0; slot < SOUND_SLOTS; slot++)
    SoundStop(slot, close);
}
