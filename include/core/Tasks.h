#ifndef Tasks_h
#define Tasks_h

void pollPower(void*);
void pollEncoder(void*);
void pollPIREvents(void*);
void pollInputBuffers(void*);
void pollRTCEvents(void*);

void emergencyBackup(void*);
void periodicBackup(void*);

void dataAppend(void*);
void dataUpdate(void*);
void plotUpdate(void*);

#endif