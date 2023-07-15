#pragma once

//comment this out to enable debugging console output
#define DBG_DISABLE

#include <time.h>

typedef struct Dbg_Timer {
	const char* name;
	clock_t start;
	char log[2048];
	int logPos;
} Dbg_Timer;

typedef struct Dbg_FPSCounter {
	int count;
	clock_t start;
	char log[256];
} Dbg_FPSCounter;

#ifndef DBG_DISABLE

void Dbg_clearConsole();

void Dbg_initTimer(Dbg_Timer*);
void Dbg_startTimer(Dbg_Timer*, const char* name);
void Dbg_stopTimer(Dbg_Timer*);
void Dbg_printTimerLog(Dbg_Timer*);
void Dbg_freeTimer(Dbg_Timer*);

void Dbg_initFPSCounter(Dbg_FPSCounter*);
void Dbg_tickFPSCounter(Dbg_FPSCounter*);
void Dbg_printFPSCounter(Dbg_FPSCounter*);
void Dbg_freeFPSCounter(Dbg_FPSCounter*);

#else

#define Dbg_clearConsole()

#define Dbg_initTimer(x)
#define Dbg_startTimer(x, y)
#define Dbg_stopTimer(x)
#define Dbg_printTimerLog(x)
#define Dbg_freeTimer(x)

#define Dbg_initFPSCounter(x)
#define Dbg_tickFPSCounter(x)
#define Dbg_printFPSCounter(x)
#define Dbg_freeFPSCounter(x)

#endif
