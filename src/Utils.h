#ifndef UTILS_H_
#define UTILS_H_

#include <arduino.h>
void startTime();
void stopTime();
void getUtcTime(struct tm * info);
void printUtcTime();
void printLocalTime();
void showDirectory();
void copyLittleFStoSD();
void checkMem();
bool printFile(String fileName);


#endif