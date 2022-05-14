void SerialOld(Boolean) SEC("aux");
Int16 SerialOnline(UInt32, UInt32, UInt16, UInt16, UInt16, UInt16, UInt16, Err *) SEC("aux");
Err SerialOffline(UInt16) SEC("aux");
Int16 SerialReceive(UInt16, char *, Int16, Err *) SEC("aux");
Int16 SerialSend(UInt16, char *, Int16, Err *) SEC("aux");
Boolean SerialCheck(UInt16) SEC("aux");
//UInt16 SerialGetStatus(UInt16) SEC("aux");
//Err SerialBreak(UInt16) SEC("aux");
