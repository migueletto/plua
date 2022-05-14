#include "p.h"
#include "serial.h"

#define msgSerialLibrary "Serial Library"
#define tamSerBuf 4096

static Int32 appSerTimeout;
static UInt16 ticksPerSecond;
static Boolean hasNewSerial;
static MemHandle serialBufferH;
static MemPtr serialBuffer;

Int16 SerialOnline(UInt32 port, UInt32 baud, UInt16 bits, UInt16 parity,
                   UInt16 stopBits, UInt16 xonXoff, UInt16 rtsCts, Err *err)
{
  SerSettingsType SerialSettings;
  UInt32 dwSerial, value, flags;
  UInt16 AppSerRefnum, len;

  *err = FtrGet(sysFileCSerialMgr, sysFtrNewSerialPresent, &dwSerial);
  hasNewSerial = *err == 0 && dwSerial != 0;

  ticksPerSecond = SysTicksPerSecond();
  appSerTimeout = ticksPerSecond/4;

  if (hasNewSerial)
    *err = SrmOpen(port, baud, &AppSerRefnum);
  else {
    if (port != serPortCradlePort ||
        SysLibFind(msgSerialLibrary, &AppSerRefnum)) {
      *err = serErrBadParam;
      return -1;
    }
    *err = SerOpen(AppSerRefnum, 0, baud);
  }

  if (*err != 0)
    return -1;
 
  if (serialBuffer == NULL) {
    if ((serialBufferH = MemHandleNew(tamSerBuf+64)) == NULL) {
      *err = memErrNotEnoughSpace;
      return -1;
    }
 
    if ((serialBuffer = MemHandleLock(serialBufferH)) == NULL) {
      MemHandleFree(serialBufferH);
      *err = memErrNotEnoughSpace;
      return -1;
    }
  }

  flags = 0;
  SerialSettings.flags = 0;
  SerialSettings.baudRate = baud;
 
  if (xonXoff) {
    SerialSettings.flags |= serSettingsFlagXonXoffM;
    flags |= srmSettingsFlagXonXoffM;
  }
 
  if (rtsCts) {
    SerialSettings.flags |= serSettingsFlagRTSAutoM;
    SerialSettings.flags |= serSettingsFlagCTSAutoM;
    flags |= srmSettingsFlagRTSAutoM;
    flags |= srmSettingsFlagCTSAutoM;
  }
 
  SerialSettings.ctsTimeout = serDefaultCTSTimeout;

  switch (bits) {
    case 5: SerialSettings.flags |= serSettingsFlagBitsPerChar5;
            flags |= srmSettingsFlagBitsPerChar5;
	    break;
    case 6: SerialSettings.flags |= serSettingsFlagBitsPerChar6;
            flags |= srmSettingsFlagBitsPerChar6;
	    break;
    case 7: SerialSettings.flags |= serSettingsFlagBitsPerChar7;
            flags |= srmSettingsFlagBitsPerChar7;
	    break;
    case 8: SerialSettings.flags |= serSettingsFlagBitsPerChar8;
            flags |= srmSettingsFlagBitsPerChar8;
  }
 
  switch (parity) {
    case 1: SerialSettings.flags |= serSettingsFlagParityOnM;
            SerialSettings.flags |= serSettingsFlagParityEvenM;
            flags |= srmSettingsFlagParityOnM;
            flags |= srmSettingsFlagParityEvenM;
	    break;
    case 2: SerialSettings.flags |= serSettingsFlagParityOnM;
            SerialSettings.flags &= ~serSettingsFlagParityEvenM;
            flags |= srmSettingsFlagParityOnM;
            flags &= ~srmSettingsFlagParityEvenM;
  }
 
  switch (stopBits) {
    case 1: SerialSettings.flags |= serSettingsFlagStopBits1;
            flags |= srmSettingsFlagStopBits1;
	    break;
    case 2: SerialSettings.flags |= serSettingsFlagStopBits2;
            flags |= srmSettingsFlagStopBits2;
  }

  if (hasNewSerial) {
    SrmSendFlush(AppSerRefnum);
    SrmReceiveFlush(AppSerRefnum, appSerTimeout);

    len = sizeof(flags);
    *err = SrmControl(AppSerRefnum, srmCtlSetFlags, &flags, &len);
    if (*err == serErrNotSupported)
      *err = 0;

    value = srmDefaultCTSTimeout;
    len = sizeof(value);
    if (*err == 0)
      *err = SrmControl(AppSerRefnum, srmCtlSetCtsTimeout, &value, &len);
    if (*err == serErrNotSupported)
      *err = 0;

  } else {
    SerSendFlush(AppSerRefnum);
    SerReceiveFlush(AppSerRefnum, appSerTimeout);

    *err = SerSetSettings(AppSerRefnum, &SerialSettings);
  }

  if (*err != 0) {
    MemHandleUnlock(serialBufferH);
    MemHandleFree(serialBufferH);
    SerClose(AppSerRefnum);
    return -1;
  }
 
  *err = hasNewSerial ?
	  SrmSetReceiveBuffer(AppSerRefnum, serialBuffer, tamSerBuf+64) :
	  SerSetReceiveBuffer(AppSerRefnum, serialBuffer, tamSerBuf+64);

  if (*err != 0) {
    MemHandleUnlock(serialBufferH);
    MemHandleFree(serialBufferH);
    SerClose(AppSerRefnum);
    return -1;
  }
 
  return AppSerRefnum;
}

Err SerialOffline(UInt16 AppSerRefnum)
{
  if (hasNewSerial)
    SrmSetReceiveBuffer(AppSerRefnum, NULL, 0);
  else
    SerSetReceiveBuffer(AppSerRefnum, NULL, 0);

  if (serialBuffer != NULL) {
    MemHandleUnlock(serialBufferH);
    MemHandleFree(serialBufferH);
    serialBuffer = NULL;
  }

  if (hasNewSerial)
    return SrmClose(AppSerRefnum);

  return SerClose(AppSerRefnum);
}

Int16 SerialReceive(UInt16 AppSerRefnum, char *buf, Int16 tam, Err *err)
{
  UInt32 n;

  *err = 0;

  if (hasNewSerial) {
    SrmClearErr(AppSerRefnum);
    if ((*err = SrmReceiveWait(AppSerRefnum, 1, appSerTimeout)) != 0)
      return -1;
 
    SrmClearErr(AppSerRefnum);
    SrmReceiveCheck(AppSerRefnum, &n);
    if (!n)
      return 0;
    if (n > tam)
      n = tam;
 
    SrmClearErr(AppSerRefnum);
    return SrmReceive(AppSerRefnum, buf, n, -1, err);
  }

  SerClearErr(AppSerRefnum);
  if ((*err = SerReceiveWait(AppSerRefnum, 1, appSerTimeout)) != 0)
    return -1;
 
  SerClearErr(AppSerRefnum);
  SerReceiveCheck(AppSerRefnum, &n);
  if (!n)
    return 0;
  if (n > tam)
    n = tam;
 
  SerClearErr(AppSerRefnum);
  return SerReceive(AppSerRefnum, buf, n, -1, err);
}

Int16 SerialSend(UInt16 AppSerRefnum, char *buf, Int16 tam, Err *err)
{
  UInt32 n;

  if (hasNewSerial) {
    SrmClearErr(AppSerRefnum);
    n = SrmSend(AppSerRefnum, buf, tam, err);
    return *err ? -1 : n;
  }

  SerClearErr(AppSerRefnum);
  n = SerSend(AppSerRefnum, buf, tam, err);
  return *err ? -1 : n;
}

Boolean SerialCheck(UInt16 AppSerRefnum)
{
  UInt32 n;
  Err err;

  if (hasNewSerial) {
    SrmClearErr(AppSerRefnum);
    err = SrmReceiveCheck(AppSerRefnum, &n);
  } else {
    SerClearErr(AppSerRefnum);
    err = SerReceiveCheck(AppSerRefnum, &n);
  }

  return err != 0 || n > 0;
}

/*
UInt16 SerialGetStatus(UInt16 AppSerRefnum)
{
  Boolean b1, b2;
  UInt16 lineErrs;
  UInt32 status;

  if (hasNewSerial)
    return SrmGetStatus(AppSerRefnum, &status, &lineErrs) ? 0 : lineErrs;

  return SerGetStatus(AppSerRefnum, &b1, &b2);
}

Err SerialBreak(UInt16 AppSerRefnum)
{
  if (hasNewSerial) {
    SrmControl(AppSerRefnum, srmCtlStartBreak, NULL, NULL);
    SysTaskDelay(3*ticksPerSecond/10);	// 300 milissegundos
    SrmControl(AppSerRefnum, srmCtlStopBreak, NULL, NULL);
  }

  SerControl(AppSerRefnum, serCtlStartBreak, NULL, NULL);
  SysTaskDelay(3*ticksPerSecond/10);	// 300 milissegundos
  SerControl(AppSerRefnum, serCtlStopBreak, NULL, NULL);

  return 0;
}
*/
