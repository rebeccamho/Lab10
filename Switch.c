// filename ******** Switch.c ************** 
// Lab 10 Spring 2017
// Matt Owens & Rebecca Ho
// 2/21/17

#include <stdint.h>
#include <stdbool.h>
#include "../ValvanoWareTM4C123/ValvanoWareTM4C123/inc/tm4c123gh6pm.h"
#include "SysTick.h"
#include "PeriodMeasure.h"
#include "PWM.h"

#define PF1             (*((volatile uint32_t *)0x40025008))
#define PF2             (*((volatile uint32_t *)0x40025010))

uint16_t DutyArray[5] = {50000,51000,52000,56000,59000};
	// values correspond to speeds ranging from 18-26 rps, incremented by 2
uint32_t SpeedArray[5] = {180,200,220,240,260};
uint16_t Current = 0;

void PortF_Init() {
	SYSCTL_RCGCGPIO_R |= 0x20;            // activate port F clock
	while((SYSCTL_PRGPIO_R&0x20)==0){}; 	// allow time for clock to start

	GPIO_PORTF_DIR_R |= 0x06;             // make PF2, PF1 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x06;          // disable alt funct on PF2, PF1

  GPIO_PORTF_DEN_R |= 0x06;             // enable digital I/O on PF4, PF2, PF1
                                        // configure PF2-1 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF00F)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF

  PF2 = 0;                  				    // turn off LED
	PF1 = 0;
}


void PortE_Init() { // switches are connected to PortE
	SYSCTL_RCGCGPIO_R |= 0x10;		// activate clock for Port E
	while((SYSCTL_PRGPIO_R&0x10)==0){}; 	// allow time for clock to start
	GPIO_PORTE_DIR_R &= ~0x30;		// make PE5-4 in
	GPIO_PORTE_DEN_R |= 0x30; 		// enable digital I/O on PE5-4
	GPIO_PORTE_IS_R &= ~0x30;			// PE5-4 is edge-sensitive
	GPIO_PORTE_IBE_R &= ~0x30;		// PE5-4 is not both edges
	GPIO_PORTE_IEV_R |= 0x30;			// PE5-4 rising edge event
	GPIO_PORTE_PDR_R |= 0x30;			// enable pull down resistors on PE5-4 
	GPIO_PORTE_ICR_R = 0x30;			// clear flag5-4
	GPIO_PORTE_IM_R |= 0x30;			// arm interrupts on PE5-4
	NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF00)|0x00000020;	// PortE=priority 1
	NVIC_EN0_R = 1<<4; 	// enable interrupt 4 in NVIC
}

void GPIOPortE_Handler(void) {
	if(GPIO_PORTE_RIS_R&0x10) {		// poll PE4, UP switch
		GPIO_PORTE_ICR_R = 0x10;		// acknowledge flag4
		SysTick_Wait10ms(10);
		if(GPIO_PORTE_DATA_R&0x10) {
			PF2 ^= 0x04;
			if(Current < 5) { // can go up in speed
				Current++;
				SetDesiredSpeed(SpeedArray[Current]);
				PWM0_Duty(DutyArray[Current]);
			}		
		}

	}
	if(GPIO_PORTE_RIS_R&0x20) {		// poll PE5, DOWN switch
		GPIO_PORTE_ICR_R = 0x20;		// acknowledge flag5
		SysTick_Wait10ms(10);
		if(GPIO_PORTE_DATA_R&0x20) {
			PF1 ^= 0x02;
			if(Current > 0) { // can go down in speed
				Current = Current - 1;
				SetDesiredSpeed(SpeedArray[Current]);
				PWM0_Duty(DutyArray[Current]);
			}
		}
	}
}


void ResetSwitches() {

}

void CheckSwitches() {

}

