//#main.c
//Matt Owens and Rebecca Ho
//April 11, 2017

#include <stdint.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "PeriodMeasure.h"
#include "PLL.h"
#include "SysTick.h"
#include "PWM.h"
#include "Switch.h"

#define F100HZ 80000000/100

uint16_t period;
uint16_t duty;

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

int main(void){           
  PLL_Init(Bus80MHz);              // 80 MHz clock
	SysTick_Init();
	PortF_Init();
	PortE_Init();										 // initialize switches
	PeriodMeasure_Init(F100HZ);			 // initialize timer0B and timer1A, call this before PWM init
	PWM0_Init(period, duty);				 // initialize PB6 for PWM0 functionality
  EnableInterrupts();
  while(1){
    WaitForInterrupt();
  }
}
