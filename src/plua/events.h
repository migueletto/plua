#ifndef _EVENTS_H
#define _EVENTS_H 1

#define ioPendingEvent          (firstUserEvent+0)
#define vmStartEvent            (firstUserEvent+1)
#define helpEvent               (firstUserEvent+2)
#define fakeKeyDownEvent        (firstUserEvent+3)
#define sampleStopEvent         (firstUserEvent+4)

Boolean TextFormHandleEvent(EventPtr);
Boolean FileFormHandleEvent(EventPtr);
Boolean PrefsFormHandleEvent(EventPtr);
Boolean CompileFormHandleEvent(EventPtr);
Boolean HelpFormHandleEvent(EventPtr);

#endif
