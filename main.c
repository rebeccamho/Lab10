//#main.c
//Matt Owens and Rebecca Ho
//April 11, 2017

#include <stdint.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "PeriodMeasure.h"
#include "PLL.h"
#include "SysTick.h"
#include "Switch.h"

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

int main(void){           
  PLL_Init(Bus80MHz);              // 80 MHz clock
  PeriodMeasure_Init();            // initialize 24-bit timer0A in capture mode
	SysTick_Init();
	PortF_Init();
	PortE_Init();										 // initialize switches

  EnableInterrupts();
  while(1){
    WaitForInterrupt();
  }
}
