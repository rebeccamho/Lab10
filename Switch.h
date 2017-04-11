// filename ******** Switch.h ************** 
// Lab 5 Spring 2017
// Matt Owens & Rebecca Ho
// 2/21/17

//------------PortF_Init------------
// Initialization of PortF, making PF1, and PF2 outputs
// and enabling PF4 to be used as a switch
// Input: none
// Output: none
void PortF_Init(void);

//------------PortE_Init------------
// Initialization of PortE, making PE5-4 inputs
// with interrupts on a rising event
// Input: none
// Output: none
void PortE_Init(void);

//------------GPIOPortE_Handler------------
// Handles interrupts on PE5-4
// Input: none
// Output: none
void GPIOPortE_Handler(void);

//------------GPIOPortF_Handler------------
// Handles interrupts PF4
// Input: none
// Output: none
void GPIOPortF_Handler(void);

//------------CheckSwitches------------
// Checks if a switch has been pressed
// Input: none
// Output: none
void CheckSwitches(void);
