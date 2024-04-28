// Lab2.c
// Runs on either MSP432 or TM4C123
// Starter project to Lab 2.  Take sensor readings, process the data,
// and output the results.  Specifically, this program will
// measure steps using the accelerometer, audio sound amplitude using
// microphone, and temperature. (we will add light back in Lab 3)
// Daniel and Jonathan Valvano
// July 12, 2016

/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to ARM Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Real-Time Operating Systems for ARM Cortex-M Microcontrollers",
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Introduction to the MSP432 Microcontroller",
   ISBN: 978-1512185676, Jonathan Valvano, copyright (c) 2016

   "Embedded Systems: Real-Time Interfacing to the MSP432 Microcontroller",
   ISBN: 978-1514676585, Jonathan Valvano, copyright (c) 2016

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

#include <stdint.h>
#include "./inc/BSP.h"
#include "./inc/Profile.h"
#include "Texas.h"
#include "./inc/CortexM.h"
#include "os.h"

uint32_t sqrt32(uint32_t s);
#define THREADFREQ 1000 // frequency in Hz of round robin scheduler

//---------------- Global variables shared between tasks ----------------
uint32_t Time;      // elasped time in 100 ms units
uint32_t Steps;     // number of steps counted
uint32_t Magnitude; // will not overflow (3*1,023^2 = 3,139,587)
                    // Exponentially Weighted Moving Average
uint32_t EWMA;      // https://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average
uint16_t SoundData; // raw data sampled from the microphone
int32_t SoundAvg;

uint32_t LightData;
int32_t TemperatureData; // 0.1C
// semaphores
int32_t NewData;            // true when new numbers to display on top of LCD
int32_t LCDmutex;           // exclusive access to LCD
int ReDrawAxes = 0;         // non-zero means redraw axes on next display task
int32_t PedestrianCrossing; // Green Light Exclusion
enum plotstate
{
  Accelerometer,
  Microphone,
  Temperature
};
enum plotstate PlotState = Accelerometer;
// color constants

//------------ end of Global variables shared between tasks -------------

//---------------- Task0 samples sound from microphone ----------------
// Event thread run by OS in real time at 1000 Hz
#define SOUNDRMSLENGTH 1000 // number of samples to collect before calculating RMS (may overflow if greater than 4104)
int16_t SoundArray[SOUNDRMSLENGTH];
// *********Task0_Init*********
// initializes microphone
// Task0 measures sound intensity
// Inputs:  none
// Outputs: none
void Task0_Init(void)
{
  BSP_Microphone_Init();
}
// *********Task0*********
// Periodic event thread runs in real time at 1000 Hz
// collects data from microphone
// Inputs:  none
// Outputs: none
void Task0(void)
{
  static int32_t soundSum = 0;
  static int time = 0; // units of microphone sampling rate

  //  ADC is shared, but on the TM4C123 it is not critical with other ADC inputs
  BSP_Microphone_Input(&SoundData);
  soundSum = soundSum + (int32_t)SoundData;
  SoundArray[time] = SoundData;
  time = time + 1;
  if (time == SOUNDRMSLENGTH)
  {
    SoundAvg = soundSum / SOUNDRMSLENGTH;
    soundSum = 0;
    OS_Signal(&NewData); // makes task5 run every 1 sec
    time = 0;
  }
}
/* ****************************************** */
/*          End of Task0 Section              */
/* ****************************************** */

// *********Task7*********
// Main thread scheduled by OS round robin preemptive scheduler
// Task7 does nothing but never blocks or sleeps
// Inputs:  none
// Outputs: none
// Use for watching Joystick Press
uint32_t Count7;
void Task7(void)
{
  Count7 = 0;
  while (1)
  {
    Count7++;
    WaitForInterrupt();
  }
}
// Check Mailbox from Task 7 and run flashing RED LEDs
// Outputs time to FIFO for Task 9
uint32_t Count8;
extern int North_South;
extern int East_West;
void Task8(void)
{
  static uint8_t prev1 = 0, prev2 = 0;
  uint8_t current;
  Count8 = 0;
  while (1)
  {
    current = BSP_Button1_Input();
    if ((current == 0) && (prev1 != 0))
    {
      East_West = -1;
    }
    prev1 = current;
    current = BSP_Button2_Input();
    if ((current == 0) && (prev2 != 0))
    {
      North_South = -1;
    }
    prev2 = current;
    Count8++;
    WaitForInterrupt();
  }
}
// Task 9 Filesystem output task that runs at low priority
// Adds time that an emergency interrupt happened to FAT

void BSP_LED_Init(void)
{
  BSP_RGB_Init(0, 0, 0);
}
int main(void)
{
  OS_Init();
  BSP_LCD_Init();
  OS_InitSemaphore(&LCDmutex, 0);
  OS_InitSemaphore(&PedestrianCrossing, 0);
  OS_MailBox_Init();
  BSP_Buzzer_Init(0);
  // BSP_LCD_FillScreen(LCD_BLACK); Synonymous with below
  BSP_LCD_FillScreen(BSP_LCD_Color565(0, 0, 0));
  BSP_LED_Init();
  Time = 0;
  BSP_Button1_Init();
  BSP_Button2_Init();
  OS_AddThreads(&HazardBuzzer, 3, &DisplayTrafficLight, 1, &Task8, 2);
  OS_AddPeriodicEventThread(&North_Light, 2000); // Period: 2000 ms
  OS_AddPeriodicEventThread(&South_Light, 2000); // Period: 2000 ms
  OS_AddPeriodicEventThread(&East_Light, 2000);  // Period: 2000 ms
  OS_AddPeriodicEventThread(&West_Light, 2000);  // Period: 2000 ms
  // OS_AddPeriodicEventThread(&Task1, 200);        // Period: 2000 ms
  AddTrafficLights();
  OS_Launch(BSP_Clock_GetFreq() / THREADFREQ); // doesn't return, interrupts enabled in here
  return 0;                                    // this never executes
}
