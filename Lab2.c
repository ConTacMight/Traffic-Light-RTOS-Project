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
uint32_t Time; // elasped time in 100 ms units

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
