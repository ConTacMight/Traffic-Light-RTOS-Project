// os.c
// Runs on LM4F120/TM4C123/MSP432
// Lab 2 starter file.
// Daniel Valvano
// February 20, 2016

#include "os.h"

static int32_t MailSend;
static volatile int32_t LostMail;
static volatile uint32_t MailData;
// function definitions in osasm.s
void StartOS(void);
TrafficLight TrafficLights[NUMLIGHTS];
eventTask_t event_tasks[NUMPERIODIC];
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];
extern uint32_t Time;
// ******** OS_Init ************
// Initialize operating system, disable interrupts
// Initialize OS controlled I/O: systick, bus clock as fast as possible
// Initialize OS global variables
// Inputs:  none
// Outputs: none
void OS_Init(void)
{
  DisableInterrupts();
  BSP_Clock_InitFastest(); // set processor clock to fastest speed
  uint8_t i;
  for (i = 0; i < NUMTHREADS; i++)
  {
    tcbs[i].blocked = NULL;
    tcbs[i].next = NULL;
    tcbs[i].sp = NULL;
    tcbs[i].sleep = 0;
    tcbs[i].WorkingPriority = 0;
    tcbs[i].FixedPriority = 0;
    tcbs[i].Age = 0;
  }

  for (i = 0; i < NUMPERIODIC; i++)
  {
    event_tasks[i].PeriodicEventTask = NULL;
    event_tasks[i].TaskPeriod = 0;
    event_tasks[i].TaskCounter = 0;
  }
  RunPt = NULL;
  BSP_PeriodicTask_Init(&runperiodicevents, TIMER_FREQ, TIMER_PRIORITY);
}

void SetInitialStack(int i)
{
  tcbs[i].sp = &Stacks[i][STACKSIZE - 16];
  Stacks[i][STACKSIZE - 1] = R16;
  Stacks[i][STACKSIZE - 3] = R14;  // R14
  Stacks[i][STACKSIZE - 4] = R12;  // R12
  Stacks[i][STACKSIZE - 5] = R3;   // R3
  Stacks[i][STACKSIZE - 6] = R2;   // R2
  Stacks[i][STACKSIZE - 7] = R1;   // R1
  Stacks[i][STACKSIZE - 8] = R0;   // R0
  Stacks[i][STACKSIZE - 9] = R11;  // R11
  Stacks[i][STACKSIZE - 10] = R10; // R10
  Stacks[i][STACKSIZE - 11] = R9;  // R9
  Stacks[i][STACKSIZE - 12] = R8;  // R8
  Stacks[i][STACKSIZE - 13] = R7;  // R7
  Stacks[i][STACKSIZE - 14] = R6;  // R6
  Stacks[i][STACKSIZE - 15] = R5;  // R5
  Stacks[i][STACKSIZE - 16] = R4;  // R4
}

//******** OS_AddThreads ***************
// Add four main threads to the scheduler
// Inputs: function pointers to four void/void main threads
// Outputs: 1 if successful, 0 if this thread can not be added
// This function will only be called once, after OS_Init and before OS_Launch
int OS_AddThreads(void (*thread0)(void), uint32_t p0,
                  void (*thread1)(void), uint32_t p1,
                  void (*thread2)(void), uint32_t p2)
{
  uint32_t crit = StartCritical();
  // initialize TCB circular list
  tcbs[0].next = &tcbs[1];
  tcbs[1].next = &tcbs[2];
  tcbs[2].next = &tcbs[0];

  for (uint8_t i = 0; i < NUMTHREADS; i++)
  {
    SetInitialStack(i);
  }

  Stacks[0][STACKSIZE - 2] = (int32_t)(thread0);
  Stacks[1][STACKSIZE - 2] = (int32_t)(thread1);
  Stacks[2][STACKSIZE - 2] = (int32_t)(thread2);

  tcbs[0].FixedPriority = tcbs[0].WorkingPriority = p0;
  tcbs[1].FixedPriority = tcbs[1].WorkingPriority = p1;
  tcbs[2].FixedPriority = tcbs[2].WorkingPriority = p2;
  // initialize RunPt
  RunPt = &tcbs[0];

  EndCritical(crit);
  return 1; // successful
}

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
int OS_AddPeriodicEventThread(void (*thread)(void), uint32_t period)
{
  uint16_t cr = StartCritical();
  static uint8_t itr = 0;
  if (itr < NUMPERIODIC)
  {
    if (event_tasks[itr].PeriodicEventTask == NULL)
    {
      event_tasks[itr].PeriodicEventTask = thread;
      event_tasks[itr].TaskPeriod = period;
      event_tasks[itr].TaskCounter = period;
      itr++;
    }
    EndCritical(cr);
    return 1;
  }
  else
    return 0;
}
void static runperiodicevents(void)
{
  // **RUN PERIODIC THREADS, DECREMENT SLEEP COUNTERS
  uint8_t i;
  for (i = 0; i < NUMTHREADS; i++)
  {
    if (tcbs[i].sleep > 0)
    {
      tcbs[i].sleep--;
    }
  }
  for (i = 0; i < NUMPERIODIC; i++)
  { // Run periodic event threads
    event_tasks[i].TaskCounter = event_tasks[i].TaskCounter--;
    if (event_tasks[i].TaskCounter == 0)
    {
      event_tasks[i].PeriodicEventTask();
      event_tasks[i].TaskCounter = event_tasks[i].TaskPeriod;
    }
  }
}
//******** OS_Launch ***************
// Start the scheduler, enable interrupts
// Inputs: number of clock cycles for each time slice
// Outputs: none (does not return)
// Errors: theTimeSlice must be less than 16,777,216
void OS_Launch(uint32_t theTimeSlice)
{
  STCTRL = 0;                                    // disable SysTick during setup
  STCURRENT = 0;                                 // any write to current clears it
  SYSPRI3 = (SYSPRI3 & 0x00FFFFFF) | 0xE0000000; // priority 7
  STRELOAD = theTimeSlice - 1;                   // reload value
  STCTRL = 0x00000007;                           // enable, core clock and interrupt arm
  StartOS();                                     // start on the first task
}

/**
 * @brief Increment the age of all tasks in the task control block (TCB) list.
 *        Decrease the working priority of tasks every 10 increments if their working priority is greater than 0.
 */
void IncrementAge(void)
{
  tcbType *ptr;
  ptr = RunPt->next;
  while (ptr != RunPt)
  {
    if (ptr->sleep == 0 && ptr->blocked == 0)
    {
      ptr->Age++;
      if (ptr->Age % 10 == 0 && ptr->WorkingPriority > 0)
      {
        ptr->WorkingPriority--;
      }
    }
    ptr = ptr->next;
  }
}
/**
 * @brief Resets the priority of a task.
 *
 * This function resets the working priority and age of a task to its fixed priority.
 *
 * @param Ptr Pointer to the task control block (tcbType) of the task.
 */
void ResetPriority(tcbType *Ptr)
{
  Ptr->WorkingPriority = Ptr->FixedPriority;
  Ptr->Age = 0;
}

/**
 * @brief Runs the scheduler function every time slice.
 *
 * This function is responsible for selecting the next thread to run based on their priorities.
 * It increments the time variable, updates the thread ages, and selects the thread with the highest working priority
 * that is not blocked or sleeping to be the next running thread.
 *
 * @param None
 * @return None
 */
// runs every ms
void Scheduler(void) // every time slice
{
  Time++;
  uint32_t maxprio = 255; // max
  tcbType *pt;
  tcbType *bpo;
  pt = RunPt;
  IncrementAge();
  do
  {
    pt = pt->next; // skips at least one
    if ((pt->WorkingPriority < maxprio) && (pt->blocked == 0) && (pt->sleep == 0))
    {
      maxprio = pt->WorkingPriority;
      bpo = pt;
    }
  } while (RunPt != pt); // look at all possible threads
  ResetPriority(bpo);
  RunPt = bpo;
}
//******** OS_Suspend ***************
// Called by main thread to cooperatively suspend operation
// Inputs: none
// Outputs: none
// Will be run again depending on sleep/block status
void OS_Suspend(void)
{
  STCURRENT = 0;        // any write to current clears it
  INTCTRL = 0x04000000; // trigger SysTick
  // next thread gets a full time slice
}
// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime)
{
  RunPt->sleep = sleepTime; // set sleep parameter in TCB, same as Lab 3
  OS_Suspend();             // suspend, stops running.
}

void OS_InitSemaphore(int32_t *semaPt, int32_t value)
{
  DisableInterrupts();
  (*semaPt) = value;
  EnableInterrupts();
}

void OS_Wait(int32_t *semaPt)
{
  DisableInterrupts();
  (*semaPt)--;
  if ((*semaPt) < 0)
  {
    RunPt->blocked = semaPt;
    EnableInterrupts();
    OS_Suspend();
  }
  EnableInterrupts();
}

void OS_Signal(int32_t *semaPt)
{
  tcbType *ptr;
  DisableInterrupts();
  (*semaPt)++;
  if ((*semaPt) <= 0)
  {
    ptr = RunPt->next;
    while (ptr->blocked != semaPt)
    {
      ptr = ptr->next;
    }
    ptr->blocked = NULL; // Wake up thread, not blocked.
  }
  EnableInterrupts();
}

void OS_MailBox_Init(void)
{
  // include data field and semaphore
  MailData = 0;
  LostMail = 0;
  OS_InitSemaphore(&MailSend, 0);
}

void OS_MailBox_Send(uint32_t data)
{
  long crit = StartCritical();
  MailData = data;
  if (MailSend)
  {
    LostMail++;
  }
  EndCritical(crit);
  OS_Signal(&MailSend);
}

uint32_t OS_MailBox_Recv(void)
{
  uint32_t data;
  OS_Wait(&MailSend);
  long crit = StartCritical();
  data = MailData;
  EndCritical(crit);
  return data;
}
int North_South = 0;
int East_West = 0;
int count = 0;
void North_Light(void)
{

  if (East_West == 0)
  {
    switch (North_South)
    {
    case 0:
      // BSP_LCD_DrawString(7, 0, "North", LCD_GREEN);
      TrafficLights[North].state = GREEN;
      break;
    case 1:
      // BSP_LCD_DrawString(7, 0, "North", LCD_GREEN);
      TrafficLights[North].state = GREEN;
      break;
    /*case 2:
      BSP_LCD_DrawString(7, 0, "North", LCD_RED);
      break;
    case 3:
      BSP_LCD_DrawString(7, 0, "North", LCD_RED);
      break;*/
    default:
      // BSP_LCD_DrawString(7, 0, "North", LCD_RED);
      TrafficLights[North].state = RED;
      break;
    }
    return;
  }
  else if (East_West == -1 && count < 3)
  {
    BSP_LCD_DrawString(13, 0, "X", LCD_YELLOW);
    TrafficLights[North].state = GREEN;
    TrafficLights[East].state = RED;
    TrafficLights[West].state = RED;
    return;
  }
  TrafficLights[North].state = RED;
}
void South_Light(void)
{

  if (East_West == 0)
  {
    switch (North_South)
    {
    case 0:
      // BSP_LCD_DrawString(7, 12, "South", LCD_GREEN);
      TrafficLights[South].state = GREEN;
      North_South++;
      break;
    case 1:
      // BSP_LCD_DrawString(7, 12, "South", LCD_RED);
      TrafficLights[South].state = RED;
      North_South++;
      break;
    case 2:
      // BSP_LCD_DrawString(7, 12, "South", LCD_GREEN);
      TrafficLights[South].state = GREEN;
      North_South++;
      break;
    case 3:
      // BSP_LCD_DrawString(7, 12, "South", LCD_RED);
      TrafficLights[South].state = RED;
      North_South = 0;
      break;
    default:
      break;
    }
    return;
  }
  else if (East_West == -1 && count < 3)
  {
    BSP_LCD_DrawString(13, 12, "X", LCD_YELLOW);
    TrafficLights[South].state = GREEN;
    TrafficLights[East].state = RED;
    TrafficLights[West].state = RED;
    count++;
    return;
  }
  else if (count >= 3)
  {
    count = 0;
    BSP_LCD_DrawString(13, 0, "X", LCD_BLACK);
    BSP_LCD_DrawString(13, 12, "X", LCD_BLACK);
    TrafficLights[North].state = RED;
    TrafficLights[South].state = RED;
    TrafficLights[East].state = RED;
    TrafficLights[West].state = RED;
    North_South = 0;
    East_West = 0;
  }
}
void East_Light(void)
{

  if (North_South == 0)
  {
    switch (East_West)
    {
    case 0:
      // BSP_LCD_DrawString(17, 6, "East", LCD_GREEN);
      TrafficLights[East].state = GREEN;
      break;
    case 1:
      // BSP_LCD_DrawString(17, 6, "East", LCD_GREEN);
      TrafficLights[East].state = GREEN;
      break;
    /*case 2:
      BSP_LCD_DrawString(17, 6, "East", LCD_RED);
      break;
    case 3:
      BSP_LCD_DrawString(17, 6, "East", LCD_RED);
      break;*/
    default:
      // BSP_LCD_DrawString(17, 6, "East", LCD_RED);
      TrafficLights[East].state = RED;
      break;
    }
    return;
  }
  else if (North_South == -1 && count < 3)
  {
    BSP_LCD_DrawString(19, 8, "X", LCD_YELLOW);
    TrafficLights[East].state = GREEN;
    TrafficLights[North].state = RED;
    TrafficLights[South].state = RED;
    return;
  }
  TrafficLights[East].state = RED;
}

void West_Light(void)
{
  if (North_South == 0)
  {
    switch (East_West)
    {
    case 0:
      // BSP_LCD_DrawString(0, 6, "West", LCD_GREEN);
      TrafficLights[West].state = GREEN;
      East_West++;
      break;
    case 1:
      // BSP_LCD_DrawString(0, 6, "West", LCD_RED);
      TrafficLights[West].state = RED;
      East_West++;
      break;
    case 2:
      // BSP_LCD_DrawString(0, 6, "West", LCD_GREEN);
      TrafficLights[West].state = GREEN;
      East_West++;
      break;
    case 3:
      // BSP_LCD_DrawString(0, 6, "West", LCD_RED);
      TrafficLights[West].state = RED;
      East_West = 0;
      break;
    default:
      break;
    }
    return;
  }
  else if (North_South == -1 && count < 3)
  {
    BSP_LCD_DrawString(0, 8, "X", LCD_YELLOW);
    TrafficLights[West].state = GREEN;
    TrafficLights[North].state = RED;
    TrafficLights[South].state = RED;
    count++;
    return;
  }
  else if (count == 3)
  {
    count = 0;
    BSP_LCD_DrawString(0, 8, "X", LCD_BLACK);
    BSP_LCD_DrawString(19, 8, "X", LCD_BLACK);
    TrafficLights[North].state = RED;
    TrafficLights[South].state = RED;
    TrafficLights[East].state = RED;
    TrafficLights[West].state = RED;
    North_South = 0;
    East_West = 0;
  }
}
void EmergencyResponse(void)
{
  for (size_t i = 0; i < 5; i++)
  {
    BSP_RGB_Set(0, 0, 1023); // Turn on red LED at full brightness
    BSP_Buzzer_Set(512);
    BSP_LCD_DrawString(7, 0, "North", LCD_RED);
    BSP_LCD_DrawString(7, 12, "South", LCD_RED);
    BSP_LCD_DrawString(17, 6, "East", LCD_RED);
    BSP_LCD_DrawString(0, 6, "West", LCD_RED);
    BSP_Delay1ms(1000);
    Time += 1000;
    BSP_RGB_Set(0, 0, 0); // Turn off LED
    BSP_Buzzer_Set(0);
    BSP_LCD_DrawString(7, 0, "North", LCD_BLACK);
    BSP_LCD_DrawString(7, 12, "South", LCD_BLACK);
    BSP_LCD_DrawString(17, 6, "East", LCD_BLACK);
    BSP_LCD_DrawString(0, 6, "West", LCD_BLACK);
    BSP_Delay1ms(1000);
    Time += 1000;
  }
  TrafficLights[North].state = RED;
  TrafficLights[South].state = RED;
  TrafficLights[East].state = RED;
  TrafficLights[West].state = RED;
  North_South = 0;
  East_West = 0;
}
extern int32_t PedestrianCrossing;
void DisplayTrafficLight(void)
{
  while (1)
  {
    // OS_Wait(&LCDmutex);
    for (size_t i = 0; i < NUMLIGHTS; i++)
    {
      uint16_t color = TrafficLightColors[TrafficLights[i].state];
      char *description = TrafficLightDirectionDescriptions[TrafficLights[i].direction];
      BSP_LCD_DrawString(TrafficLights[i].posx, TrafficLights[i].posy, description, color);
    }
    // OS_Signal(&LCDmutex);
    OS_Sleep(500);
  }
}
void AddTrafficLights(void)
{
  BSP_LCD_DrawFastHLine(10, 100, 120, LCD_WHITE); // North Horizontal line
  BSP_LCD_DrawFastHLine(10, 20, 120, LCD_WHITE);  // South Horizontal line
  BSP_LCD_DrawFastVLine(95, 10, 120, LCD_WHITE);  // East Vertical line
  BSP_LCD_DrawFastVLine(30, 10, 120, LCD_WHITE);  // West Vertical line
  BSP_LCD_DrawString(7, 0, "North", LCD_RED);
  TrafficLights[North].direction = North;
  TrafficLights[North].state = RED;
  TrafficLights[North].posx = 7;
  TrafficLights[North].posy = 0;
  BSP_LCD_DrawString(7, 12, "South", LCD_RED);
  TrafficLights[South].direction = South;
  TrafficLights[South].state = RED;
  TrafficLights[South].posx = 7;
  TrafficLights[South].posy = 12;
  BSP_LCD_DrawString(17, 6, "East", LCD_RED);
  TrafficLights[East].direction = East;
  TrafficLights[East].state = RED;
  TrafficLights[East].posx = 17;
  TrafficLights[East].posy = 6;
  BSP_LCD_DrawString(0, 6, "West", LCD_RED);
  TrafficLights[West].direction = West;
  TrafficLights[West].state = RED;
  TrafficLights[West].posx = 0;
  TrafficLights[West].posy = 6;
}
uint16_t Joyx, Joyy;
uint8_t JoystickPress;
void HazardBuzzer(void)
{
  BSP_Joystick_Init();
  uint8_t currentButtonState;

  // BSP_Buzzer_Init(0); // Initialize the buzzer with 0 duty cycle (off)

  while (1)
  {
    BSP_Joystick_Input(&Joyx, &Joyy, &JoystickPress);
    currentButtonState = JoystickPress; // Read the current state of the button
    if (currentButtonState == 0)
    { // Check for button press
      DisableInterrupts();
      EmergencyResponse();
      EnableInterrupts();
    }
    OS_Sleep(2000);
  }
}

// Newton's method
// s is an integer
// sqrt(s) is an integer
uint32_t sqrt32(uint32_t s)
{
  uint32_t t;     // t*t will become s
  int n;          // loop counter
  t = s / 16 + 1; // initial guess
  for (n = 16; n; --n)
  { // will finish
    t = ((t * t + s) / t) / 2;
  }
  return t;
}
