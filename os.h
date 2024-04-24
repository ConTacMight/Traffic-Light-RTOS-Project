// os.h
// Runs on LM4F120/TM4C123/MSP432
// A very simple real time operating system with minimal features.
// Daniel Valvano
// February 20, 2016

/* This example accompanies the book

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
   ISBN: 978-1466468863, , Jonathan Valvano, copyright (c) 2016
   Programs 4.4 through 4.12, section 4.2

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

#ifndef __OS_H
#define __OS_H 1
#include <stdint.h>
#include <stdio.h>
#include "./inc/CortexM.h"
#include "./inc/BSP.h"
#define NUMTHREADS 2  // maximum number of threads
#define STACKSIZE 100 // number of 32-bit words in stack per thread
#define PERIODIC_TASKS_NUM 1
#define NULL_PTR ((void *)0) // Null pointer
#define NUMPERIODIC 1
#define TIMER_FREQ 1000
#define TIMER_PRIORITY 6
#define NUMLIGHTS 2

#define BGCOLOR LCD_BLACK
#define AXISCOLOR LCD_ORANGE
#define MAGCOLOR LCD_YELLOW
#define EWMACOLOR LCD_CYAN
#define SOUNDCOLOR LCD_CYAN
#define TEMPCOLOR LCD_LIGHTGREEN
#define TOPTXTCOLOR LCD_WHITE
#define TOPNUMCOLOR LCD_ORANGE

#define R0 0x00000000
#define R1 0x01010101
#define R2 0x02020202
#define R3 0x03030303
#define R4 0x04040404
#define R5 0x05050505
#define R6 0x06060606
#define R7 0x07070707
#define R8 0x08080808
#define R9 0x09090909
#define R10 0x10101010
#define R11 0x11111111
#define R12 0x12121212
#define R13 0x13131313
#define R14 0x14141414
#define R15 0x15151515 // PC register.
#define R16 0x01000000 // Thumb bit register - PSR.
struct tcb
{
  int32_t *sp;      // pointer to stack (valid for threads not running
  struct tcb *next; // linked-list pointer
  int32_t sleep;    // nonzero if this thread is sleeping
  int32_t *blocked; // nonzero if blocked on this semaphore
};
typedef struct eventTask
{
  void (*PeriodicEventTask)(void);
  uint32_t TaskPeriod;
  uint32_t TaskCounter;
} eventTask_t, *eventTaskPt;
typedef enum
{
  RED,
  GREEN,
} TrafficLightState;

typedef struct
{
  int pair;
  TrafficLightState state;
  int timer, cars;
} TrafficLightPair;

// ******** OS_Init ************
// Initialize operating system, disable interrupts
// Initialize OS controlled I/O: systick, bus clock as fast as possible
// Initialize OS global variables
// Inputs:  none
// Outputs: none
void OS_Init(void);

//******** OS_AddThreads ***************
// Add four main threads to the scheduler
// Inputs: function pointers to four void/void main threads
// Outputs: 1 if successful, 0 if this thread can not be added
// This function will only be called once, after OS_Init and before OS_Launch
int OS_AddThreads(void (*thread0)(void), void (*thread1)(void));

//******** OS_AddPeriodicEventThreads ***************
// Add two background periodic event threads
// Typically this function receives the highest priority
// Inputs: pointers to a void/void event thread function2
//         periods given in units of OS_Launch (Lab 2 this will be msec)
// Outputs: 1 if successful, 0 if this thread cannot be added
// It is assumed that the event threads will run to completion and return
// It is assumed the time to run these event threads is short compared to 1 msec
// These threads cannot spin, block, loop, sleep, or kill
// These threads can call OS_Signal
int OS_AddPeriodicEventThread(void (*thread)(void), uint32_t period);

//******** OS_Launch ***************
// Start the scheduler, enable interrupts
// Inputs: number of clock cycles for each time slice
// Outputs: none (does not return)
// Errors: theTimeSlice must be less than 16,777,216
void OS_Launch(uint32_t theTimeSlice);
void OS_Sleep(uint32_t sleepTime);
// ******** OS_InitSemaphore ************
// Initialize counting semaphore
// Inputs:  pointer to a semaphore
//          initial value of semaphore
// Outputs: none
void OS_InitSemaphore(int32_t *semaPt, int32_t value);

// ******** OS_Wait ************
// Decrement semaphore
// Lab2 spinlock (does not suspend while spinning)
// Lab3 block if less than zero
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Wait(int32_t *semaPt);

// ******** OS_Signal ************
// Increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Signal(int32_t *semaPt);

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Producer is an event thread, consumer is a main thread
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void);

// ******** OS_MailBox_Send ************
// Enter data into the MailBox, do not spin/block if full
// Use semaphore to synchronize with OS_MailBox_Recv
// Inputs:  data to be sent
// Outputs: none
// Errors: data lost if MailBox already has data
void OS_MailBox_Send(uint32_t data);

// ******** OS_MailBox_Recv ************
// retreive mail from the MailBox
// Use semaphore to synchronize with OS_MailBox_Send
// Lab 2 spin on semaphore if mailbox empty
// Lab 3 block on semaphore if mailbox empty
// Inputs:  none
// Outputs: data retreived
// Errors:  none
uint32_t OS_MailBox_Recv(void);
void static runperiodicevents(void);
void AddTrafficLights(void);
void UpdateTrafficLights(int pairnumber, TrafficLightState state);
void AddTrafficLights(void);
void SwitchTrafficLightTask(void);
#endif
