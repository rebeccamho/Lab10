// PeriodMeasure.c
// Runs on LM4F120/TM4C123
// Use Timer0A and B in 24-bit edge time mode to request interrupts on the rising
// edge of PB7 (T0CCP1), and measure period between pulses.
// Daniel Valvano
// May 5, 2015
// edited by Rebecca Ho and Matt Owens April 11, 2017

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015
   Example 7.2, Program 7.2

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
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

// external signal connected to PB6 (T0CCP0) (trigger on rising edge)
#include <stdint.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "PLL.h"

#define NVIC_EN0_INT19          0x00080000  // Interrupt 19 enable
#define PF1             (*((volatile uint32_t *)0x40025008))
#define PF2                     (*((volatile uint32_t *)0x40025010))
#define TIMER_TAMR_TACMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TAMR_TAMR_CAP     0x00000003  // Capture mode
#define TIMER_CTL_TAEN          0x00000001  // GPTM TimerA Enable
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge
#define TIMER_CTL_TAEVENT_NEG   0x00000004  // Negative edge
#define TIMER_CTL_TAEVENT_BOTH  0x0000000C  // Both edges
#define TIMER0B_IMR_CAEIM         0x00000040  // GPTM CaptureB Event Interrupt
                                            // Mask
#define TIMER0B_ICR_CAECINT       0x00000040  // GPTM CaptureB Event Interrupt
                                            // Clear
#define TIMER_TAILR_TAILRL_M    0x0000FFFF  // GPTM TimerA Interval Load
                                            // Register Low
#define F100HZ 80000000/100

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

uint32_t Period;              // (1/clock) units
uint32_t First;               // Timer0A first edge
int32_t Done;                 // set each rising
uint32_t Speed;								// 0.1 rps
uint32_t Count;

// max period is (2^24-1)*12.5ns = 209.7151ms
// min period determined by time to run ISR, which is about 1us
void PeriodMeasure_Init(void){
  long sr = StartCritical(); 
  SYSCTL_RCGCTIMER_R |= 0x01;     // activate timer0
  SYSCTL_RCGCGPIO_R |= 0x02;      // activate port B
	First = 0;
  Done = 0;
	Count = 0;
  GPIO_PORTB_DIR_R &= ~0x80;      // make PB7 input
  GPIO_PORTB_DEN_R |= 0x80;       // enable digital PB7
  GPIO_PORTB_AFSEL_R |= 0x80;     // enable alt funct on PB7
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0x0FFFFFFF)+0x70000000;
  TIMER0_CTL_R = 0x00000000;      // disable TIMER0A TIMER0B during setup
  TIMER0_CFG_R = 0x00000004;      // configure for 16-bit mode
  TIMER0_TAMR_R = 0x00000002;     // configure for periodic mode, default down-count settings
  TIMER0_TAILR_R = F100HZ;        // reload value 100 Hz
  TIMER0_TAPR_R = 79;             // 1us resolution
  TIMER0_TBMR_R = 0x00000003;     // edge count mode
  // bits 11-10 are 0 for rising edge counting on PB7
  TIMER0_TBILR_R = 0xFFFFFFFF;    // start value
  TIMER0_TBPR_R = 0xFF;           // activate prescale, creating 24-bit 
  //TIMER0_IMR_R &= ~0x700;         // disable all interrupts for timer0B
  TIMER0_IMR_R |= TIMER0B_IMR_CAEIM;						// enable input capture interrupts for timer0B
	TIMER0_ICR_R |= (0x00000001|TIMER0B_ICR_CAECINT);      // clear TIMER0A-B timeout flags
  TIMER0_IMR_R = (0x00000001|TIMER0B_IMR_CAEIM);      // arm timeout interrupt for timer 0A, enable input capture for timer0B
	// interrupts enabled in the main program after all devices initialized
	// timer0a vector number 35, interrupt number 19
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x80000000; // timer0A priority 4
	NVIC_EN0_R = 1<<19;             // enable IRQ 19 in NVIC
	// timer 0b vector number 36, interrupt number 20
	NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFFFF00)|0x00000040; // timer0B priority 3
	NVIC_EN0_R = 1<<20;							// enable IRQ 20 in NVIC
  TIMER0_CTL_R |= 0x00000101;     // enable TIMER0A TIMER0B
  EndCritical(sr);
}
void Timer0A_Handler(void){
  PF2 = PF2^0x04;  // toggle PF2
  PF2 = PF2^0x04;  // toggle PF2
  TIMER0_ICR_R |= 0x00000001;      // acknowledge timer0A timeout
	Count++;
	long sr = StartCritical();
  if(Count <= 3 && (Period > 200000) && (Period < 2000000)) { // 10rps < period < 100rps
		Speed = 200000000/Period; // 0.1 rps
		Count = 0;
	} else { // motor is off
		Speed = 0;
	}
  EndCritical(sr);
  
  PF2 = PF2^0x04;  // toggle PF2
}

void Timer0B_Handler(void) {
	PF1 ^= 0x02;
	PF1 ^= 0x02;
	TIMER0_ICR_R |= TIMER0B_ICR_CAECINT;	// acknoledge timer0b timeout
	Done = 0;
	Count = 0;
	Period = (First - TIMER0_TBR_R)&0xFFFFFF; // 24 bits, 12.5ns resolution
	First = TIMER0_TBR_R; // setup for next
	Done = 1;
	
	PF1 ^= 0x02;
}