#ifndef Tasks_h
#define Tasks_h

void blink(void*);

void pollPower(void*);
void pollEncoder(void*);
void pollRTCEvents(void*);
void pollPIREvents(void*);
void pollInputBuffers(void*);

void emergencyBackup(void*);
void periodicBackup(void*);

void dataAppend(void*);
void dataUpdate(void*);
void plotUpdate(void*);

#endif