///////////////////////////////////////////////////////////////////
//Student ID No.:   815009698
//Date:             1/11/2017
///////////////////////////////////////////////////////////////////

#include "includes.h" 
#include <timers.h> 
#include "xlcd.h"
#include <delays.h>
#include <string.h>
#include <stdlib.h>

static char taskA_welcome[17L]="Task 1 rocks! \n";
static char taskB_welcome[17L]="Task 2 rules?";
unsigned char stopped = 0;

static char timestamp[10];
INT32U timestampint;
INT8U QueueNumber;
static char QueueMessage;
OS_Q_DATA* QueueData;
OS_EVENT* Queue;

OS_EVENT* Sem;
OS_EVENT* CountingSem;
OS_EVENT* FDirSem;
OS_EVENT* BDirSem;

void* myArrayofMsgs[1];

/* Write the appropriate code to set configuration bits: 
 * - set HS oscillator 
 * - disable watchdog timer 
 * - disable low voltage programming 
 */
#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF

#define _XTAL_FREQ 4000000


/* Write LCD delay functions */ 
void DelayFor18TCY( void ) 
{ 
    Delay1TCY();
 	Delay1TCY();
    Delay1TCY();
    Delay1TCY();
 	Delay1TCY();
    Delay1TCY();
 	Delay1TCY();
 	Delay1TCY();
 	Delay10TCYx(1L);
}

void DelayPORXLCD (void) 
{ 
    Delay1KTCYx(15L);		// Delay of 15ms
                            // Cycles = (TimeDelay * Fosc) / 4
                            // Cycles = (15ms * 4MHz) / 4
                            // Cycles = 15,000
}

void DelayXLCD (void) 
{ 
    Delay1KTCYx(5L); 		// Delay of 5ms
                        // Cycles = (TimeDelay * Fosc) / 4
                        // Cycles = (5ms * 4MHz) / 4
                        // Cycles = 5,000
}

/* Write the appropriate code to do the following:
* define the stack sizes for task1 and task2 in app_cfg.h file
* use the defines to assign stack sizes here.
*/
OS_STK TaskAStk[TaskAStkSize];
OS_STK TaskBStk[TaskBStkSize];

/* Write the appropriate code to do the following:
* Configure PORTB pin 1 as an output
* TaskA will loop until the global variable stopped is set.
* Within the loop   -- print string "Task 1 rocks! \n" to LCD top row
*                   -- toggle PORTB pin 1
*                   -- delay for 1 second using OSTimeDlyHMSM( )
* TaskA will delete itself if the loop is exited.
*/
void TaskA(void *pdata)
{
    auto INT8U  err;
    
    TRISBbits.TRISB1 = 0;  
    
    while (stopped == 0)
    {
        OSSemPend(Sem, 0, &err);
        while(BusyXLCD());              
        SetDDRamAddr(0x00);               
        putsXLCD(taskA_welcome);
                
        PORTBbits.RB1 = !PORTBbits.RB1;
        
        //OSTimeDlyHMSM ( 0, 0, 1, 0); 
        OSSemPend(FDirSem, 0, &err);        
        WriteCmdXLCD(0b00000001); // display clear
        OSSemPost(Sem);
        OSSemPost(BDirSem);
        
    }
    OSTaskDel(OS_PRIO_SELF); 
}

/* Write the appropriate code to do the following:
* Configure PORTB pin 2 as an output
* TaskB will loop until the global variable stopped is set.
* Within the loop   -- print string "Task 2 rules?\n" to LCD bottom row
*                   -- toggle PORTB pin 2
*                   -- delay of 200 ticks using OSTimeDly( )
* TaskB will delete itself if the loop is exited.
*/
void TaskB(void *pdata)
{
    auto INT8U  err;
    
    TRISBbits.TRISB2 = 0;  
    
    while (stopped == 0)
    {
        OSSemPend(BDirSem, 0, &err);
        OSSemPend(Sem, 0, &err);
        while(BusyXLCD());             
        SetDDRamAddr(0x40);                
        putsXLCD(timestamp);
        SetDDRamAddr(0x10);
        OSQQuery(Queue,QueueData);
        QueueNumber = QueueData -> OSNMsgs;
        itoa(QueueNumber, QueueMessage);
        putsXLCD(QueueMessage);
        OSTimeDly(200);
        OSQFlush(Queue);    
        PORTBbits.RB2 = !PORTBbits.RB2;
        
         
        QueueNumber = QueueData -> OSNMsgs;
        itoa(QueueNumber, QueueMessage);
        putsXLCD(QueueMessage);
        
        
        //OSSemPend(CountingSem, 0, &err);
        //WriteCmdXLCD(0b00000001); // display clear
        OSSemPost(Sem);
        OSSemPost(FDirSem);
    }
    OSTaskDel(OS_PRIO_SELF); 
}

void appISR( void )
{
    if (PIR1bits.TMR1IF)            //check if timer1 overflowed
    {
        PIR1bits.TMR1IF = 0;        //reset it
        WriteTimer1( 0x3BE0 );      //set value in timer to expire in 200ms
        timestampint = OSTimeGet();
        ultoa(timestampint, timestamp);
        OSQPost(Queue, timestamp);
        //OSSemPost(CountingSem);     //release semaphore to signal to taskB
        OSSemPost(Sem);
    }
}

void main (void)
{
    //PORTBbits.RB2 = 0;
    //PORTBbits.RB1 = 0;
    // Write the appropriate code to disable all interrupts
    INTCONbits.GIEH = 0;
    // Write the appropriate code to initialise uC/OS II kernel
    OSInit();
    /* Write the appropriate code to enable timer0,
    * using 16 bit mode on internal clk source and pre-scalar of 1.
    */
    OpenTimer0(TIMER_INT_ON & T0_16BIT & T0_SOURCE_INT & T0_PS_1_1);
    /* Write the appropriate code to set timer0 registers
    * such that timer0 expires at 10ms using 4 Mhz oscillator.
    * Do the same in vectors.c in CPUlowInterruptHook( ).
    */
    WriteTimer0(4377);
    
    OpenTimer1(TIMER_INT_ON & T1_16BIT_RW & T1_SOURCE_INT & T1_PS_1_4 & T1_OSC1EN_ON & T1_SYNC_EXT_OFF);
    WriteTimer1(0x3BE0);
    
    /* Write the appropriate code to define the priorities for taskA
    * and taskB in app_cfg.h. Use these defines to assign priorities
    * when creating taskA and taskB with OSTaskCreate( )
    */
    
    OSTaskCreate(TaskA, (void *)0, &TaskAStk[0], TaskAPrio); //Creating TaskA Task
    OSTaskCreate(TaskB, (void *)0, &TaskBStk[0], TaskBPrio); //Creating TaskB Task
    Queue = OSQCreate(myArrayofMsgs[1],1);
    Sem = OSSemCreate(1);
    CountingSem = OSSemCreate(0);
    FDirSem = OSSemCreate(1);
    BDirSem = OSSemCreate(1);
    
    // Initialize the LCD display 
    TRISD =0x00;
    PORTD =0x00;
    OpenXLCD(FOUR_BIT & LINES_5X7);
    while(BusyXLCD());
    WriteCmdXLCD(DON & CURSOR_OFF & BLINK_OFF); // display on
    while(BusyXLCD());
    WriteCmdXLCD(0b00000001); // display clear
    while(BusyXLCD());
    WriteCmdXLCD(ENTRY_CURSOR_INC & ENTRY_DISPLAY_NO_SHIFT); // entry mode
    // Write the appropriate code to start uC/OS II kernel
    OSStart();
}