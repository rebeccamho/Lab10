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
#include <stdbool.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "PWM.h"
#include "ST7735.h"

#define PF1             (*((volatile uint32_t *)0x40025008))
#define PF2                     (*((volatile uint32_t *)0x40025010))

#define TIMER0B_IMR_CAEIM       0x00000400  // GPTM CaptureB Event Interrupt Mask
#define TIMER_TBMR_TBCMR        0x00000004  // GPTM TimerA Capture Mode
#define TIMER_TBMR_TBMR_CAP     0x00000003  // Capture mode

#define TIMER0B_ICR_CAECINT     0x00000400  // GPTM CaptureB Event Interrupt Clear
#define TIMER_CTL_TAEVENT_POS   0x00000000  // Positive edge


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

uint32_t Period;              // (1/clock) units
uint32_t First;               // Timer0A first edge
uint32_t Speed;								// 0.1 rps
uint32_t Count;
uint32_t DesiredSpeed;				// 0.1 rps
int32_t E;										// speed error in 0.1 rps
int32_t U;										// duty cycle, 40 to 39960
bool NewSpeed;


// max period is (2^24-1)*12.5ns = 209.7151ms
// min period determined by time to run ISR, which is about 1us
void PeriodMeasure_Init(uint32_t period){
  long sr = StartCritical(); 
	
  SYSCTL_RCGCTIMER_R |= 0x03;     // activate timer0 and timer1
  SYSCTL_RCGCGPIO_R |= 0x02;      // activate port B
	while((SYSCTL_PRGPIO_R&0x02) == 0) {};
	
	First = 0;
	Count = 0;
	NewSpeed = false;
	
	// PB7 Init
  GPIO_PORTB_DIR_R &= ~0x80;      // make PB7 input
  GPIO_PORTB_DEN_R |= 0x80;       // enable digital PB7
  GPIO_PORTB_AFSEL_R |= 0x80;     // enable alt funct on PB7
  GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R&0x0FFFFFFF)+0x70000000;
	GPIO_PORTB_AMSEL_R &= ~0x80;     // disable analog functionality on PB7
	
	// TIMER1A Init
	TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000004;    // 2) configure for 16-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = period-1;    // 4) reload value
  TIMER1_TAPR_R = 79;           // 5) 1us resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 8) priority 4
			// interrupts enabled in the main program after all devices initialized
			// vector number 37, interrupt number 21
  NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
		
	// TIMER0B init
  TIMER0_CTL_R = 0x00000000;      // disable TIMER0B during setup
  TIMER0_CFG_R = 0x00000004;      // configure for 16-bit mode
  TIMER0_TBMR_R = (0x00000003|0x00000004);     // edge count mode
			// bits 11-10 are 0 for rising edge counting on PB7
	                                   // configure for rising edge event
  TIMER0_CTL_R &= ~(TIMER_CTL_TAEVENT_POS|0xC00);
  TIMER0_TBILR_R = 0xFFFFFFFF;    // start value
  TIMER0_TBPR_R = 0xFF;           // activate prescale, creating 24-bit 
			//TIMER0_IMR_R &= ~0x700;         // disable all interrupts for timer0B, this line was removed from original code
  TIMER0_IMR_R |= TIMER0B_IMR_CAEIM;			// enable input capture interrupts for timer0B
	TIMER0_ICR_R = TIMER0B_ICR_CAECINT;    // clear TIMER0A-B timeout flags
 // TIMER0_IMR_R |= TIMER0B_IMR_CAEIM;      // arm timeout interrupt for timer 0A, enable input capture for timer0B
			// interrupts enabled in the main program after all devices initialized
			// timer 0b vector number 36, interrupt number 20
	NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFFFF00)|0x00000040; // timer0B priority 3
	NVIC_EN0_R = 1<<20;							// enable IRQ 20 in NVIC

	TIMER1_CTL_R = 0x00000001;    	// enable TIMER1A
	TIMER0_CTL_R |= 0x00000100;     // enable TIMER0B
  EndCritical(sr);
}



void Timer1A_Handler(void){
	PF2 = PF2^0x04;  // toggle PF2
  PF2 = PF2^0x04;  // toggle PF2
  TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge TIMER1A timeout
	
	Count++;
	long sr = StartCritical();
  if(Count <= 3 && (Period > 200000) && (Period < 2000000)) { // 10rps < period < 100rps
		Speed = 200000000/Period; // 0.1 rps
		Count = 0;
	} else { // motor is off
		Speed = 0;
	}
  EndCritical(sr);

	/*
	E = DesiredSpeed - Speed;
	U = U + (3*E)/64;
	if(U < 30000) { U = 30000; }
	if(U > 39960) { U = 39960; }
	ST7735_OutUDec(U);
	PWM0_Duty(U);
	*/
	
	
	NewSpeed = true;
  
  PF2 = PF2^0x04;  // toggle PF2
}

void Timer0B_Handler(void) {
	PF1 ^= 0x02;
	PF1 ^= 0x02;
	TIMER0_ICR_R |= TIMER0B_ICR_CAECINT;	// acknoledge timer0b timeout
	
	Count = 0;
	Period = (First - TIMER0_TBR_R)&0xFFFFFF; // 24 bits, 12.5ns resolution
	First = TIMER0_TBR_R; // setup for next
	
	PF1 ^= 0x02;
}

bool CheckIfNewSpeed(uint32_t* speed, uint16_t* duty) {
	if(NewSpeed) {
		NewSpeed = false;
		*speed = Speed;
		E = DesiredSpeed - Speed;
		U = (U*100 + (3*E)*100/64)/100;
		if(U < 30000) { U = 30000; }
		if(U > 39960) { U = 39960; }
		PWM0_Duty(U);
		*duty = U;
		return true;
	}
	return false;
}

void SetDesiredSpeed(uint32_t speed) {
	DesiredSpeed = speed;
}

void SetDuty(int32_t u) {
	U = u;
}

