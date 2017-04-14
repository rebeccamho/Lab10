//#main.c
//Matt Owens and Rebecca Ho
//April 11, 2017

#include <stdint.h>
#include <stdbool.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "PeriodMeasure.h"
#include "PLL.h"
#include "SysTick.h"
#include "PWM.h"
#include "Switch.h"
#include "ST7735.h"

#define F100HZ 80000000/100
#define F1HZ 	 80000000/2
#define fs 100
#define N 1

uint16_t PeriodInit = 60001;
uint16_t DutyInit = 50000; // 30000 to 60000
uint32_t DesiredSpeedMain = 180; // 0.1 rps

// 18 rps: 50000
// 20 rps: 51000
// 22 rps: 52000
// 24 rps: 56000
// 26 rps: 59000

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void ChangeMainDesiredSpeed(uint32_t speed) {
	DesiredSpeedMain = speed;
}

int main(void){           
  PLL_Init(Bus80MHz);              // 80 MHz clock
	Output_Init();									 // initialize ST7735

	ST7735_FillScreen(ST7735_BLACK); 
	ST7735_SetCursor(0,0); 
	SysTick_Init();
	PortF_Init();
	PortE_Init();										 // initialize switches
	PeriodMeasure_Init(F100HZ);			 // initialize timer0B and timer1A, call this before PWM init
	PWM0_Init(PeriodInit, DutyInit);				 // initialize PB6 for PWM0 functionality
	SetDesiredSpeed(DesiredSpeedMain);						 // set initial desired speed to 200
	SetDuty(DutyInit);
	EnableInterrupts();
 

	ST7735_FillScreen(ST7735_BLACK); 
	ST7735_SetCursor(0,0); 
	ST7735_SetCursor(0,0); ST7735_OutString("Motor Lab 10");
	ST7735_PlotClear(150,300);  // range from 0 to 4095
	ST7735_SetCursor(0,1); ST7735_OutString("Duty=");
	ST7735_SetCursor(0,2); ST7735_OutString("Speed="); ST7735_sDecOut2(2500);
                        ST7735_OutString(" rps");
	uint32_t speed;
	uint32_t desired_speed;
	uint16_t duty;
	uint32_t j = 0;
	while(1){		
    WaitForInterrupt();
		
		if(CheckIfNewSpeed(&speed,&duty,&desired_speed)) { // new speed value was returned		
			//ST7735_OutUDec(speed);
			//ST7735_sDecOut1(speed);
			//ST7735_SetCursor(0,0); 
			ST7735_PlotPointWithColor(desired_speed, ST7735_BLUE);  // Measured speed
			ST7735_PlotPointWithColor(speed, ST7735_BLACK);  // Measured speed
			if((j&(N-1))==0){          // fs sampling, fs/N samples plotted per second
				ST7735_PlotNextErase();  // overwrites N points on same line
			}
			if((j%fs)==0){    // fs sampling, 1 Hz display of numerical data
				ST7735_SetCursor(6,1); ST7735_OutUDec(duty);            // 0 to 4095
				ST7735_SetCursor(7,2); ST7735_sDecOut1(speed); // 0.01 C 
			}
			j++;                       // counts the number of samples
		}
		
  }
}
