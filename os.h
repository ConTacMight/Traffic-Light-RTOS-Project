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
#define NUMTHREADS 3         // maximum number of threads
#define STACKSIZE 100        // number of 32-bit words in stack per thread
#define NULL_PTR ((void *)0) // Null pointer
#define NUMPERIODIC 4
#define TIMER_FREQ 1000
#define TIMER_PRIORITY 6
#define NUMLIGHTS 4
#define BGCOLOR LCD_BLACK
#define AXISCOLOR LCD_ORANGE
#define MAGCOLOR LCD_YELLOW
#define EWMACOLOR LCD_CYAN
#define SOUNDCOLOR LCD_CYAN
#define TEMPCOLOR LCD_LIGHTGREEN
#define TOPTXTCOLOR LCD_WHITE
#define TOPNUMCOLOR LCD_ORANGE

#define LOCALCOUNTTARGET 5 // The number of valid measured magnitudes needed to confirm a local min or local max.  Increase this number for longer strides or more frequent measurements.
#define AVGOVERSHOOT 25    // The amount above or below average a measurement must be to count as "crossing" the average.  Increase this number to reject increasingly hard shaking as steps.
#define ACCELERATION_MAX 1400
#define ACCELERATION_MIN 600
#define ALPHA 128 // The degree of weighting decrease, a constant smoothing factor between 0 and 1,023. A higher ALPHA discounts older observations faster.
                  // basic step counting algorithm is based on a forum post from
                  // http://stackoverflow.com/questions/16392142/android-accelerometer-profiling/16539643#16539643
#define SOUND_MAX 900
#define SOUND_MIN 300
#define LIGHT_MAX 200000
#define LIGHT_MIN 0
#define TEMP_MAX 1023
#define TEMP_MIN 0

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
static const uint16_t TrafficLightColors[] = {
    LCD_RED,  // RED
    LCD_GREEN // GREEN
};
static char *TrafficLightDirectionDescriptions[] = {
    "North", // North
    "East",  // East
    "South", // South
    "West"   // West
};
typedef enum
{
  RED,
  GREEN
} TrafficLightState;
typedef enum
{
  North,
  East,
  South,
  West
} TrafficLightDirection;
struct tcb
{
  int32_t *sp;             // pointer to stack (valid for threads not running
  struct tcb *next;        // linked-list pointer
  int32_t sleep;           // nonzero if this thread is sleeping
  int32_t *blocked;        // nonzero if blocked on this semaphore
  uint8_t WorkingPriority; // used by the scheduler
  uint8_t FixedPriority;   // permanent priority
  uint32_t Age;            // time since last execution
};
typedef struct eventTask
{
  void (*PeriodicEventTask)(void);
  uint32_t TaskPeriod;
  uint32_t TaskCounter;
} eventTask_t, *eventTaskPt;

typedef struct
{
  int posx, posy;
  TrafficLightState state;
  TrafficLightDirection direction;
  int timer, cars;
  int crossing;
} TrafficLight;

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
int OS_AddThreads(void (*thread0)(void), uint32_t p0,
                  void (*thread1)(void), uint32_t p1,
                  void (*thread2)(void), uint32_t p2);

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

//******** OS_Suspend ***************
// Called by main thread to cooperatively suspend operation
// Inputs: none
// Outputs: none
// Will be run again depending on sleep/block status
void OS_Suspend(void);

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// OS_Sleep(0) implements cooperative multitasking

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

/**
 * @brief Executes the periodic events in the operating system.
 *
 * This function is responsible for running the periodic events in the operating system.
 * It should be called periodically to ensure that the system tasks are executed at the
 * desired intervals.
 */
void static runperiodicevents(void);

/**
 * @brief Adds traffic lights to the system.
 *
 * This function is responsible for adding traffic lights to the system.
 * It performs the necessary initialization and configuration for the traffic lights.
 * JPK
 * @return void
 */
void AddTrafficLights(void);

/**
 * @brief Controls the north traffic light.
 * Acceses the Traffic Light Array Struct.
 * JPK
 */
void North_Light(void);

/**
 * @brief Controls the various south traffic light states with switches.
 * Acceses the Traffic Light Array Struct. Master.
 * JPK
 */
void South_Light(void);

/**
 * @brief Controls the east traffic light.
 * Acceses the Traffic Light Array Struct.
 * JPK
 */
void East_Light(void);

/**
 * @brief Controls the west traffic light.
 * Acceses the Traffic Light Array Struct. Master.
 * JPK
 */
void West_Light(void);
void HazardBuzzer(void);
/**
 * @brief Displays the traffic light.
 *
 * This thread is responsible for displaying the traffic light on a display device.
 * It is called to update the state of the traffic light based on the current traffic conditions.
 * Acceses the Traffic Light Array Struct.
 * JPK
 *
 */
void DisplayTrafficLight(void);

uint32_t sqrt32(uint32_t s);
void EmergencyResponse(void);
void PedestrianCross(void);
#endif
