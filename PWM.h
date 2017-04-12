//#PWM.h
//Matt Owens and Rebecca Ho
//April 11, 2017


//------------PWM0_Init------------
// Initialization of alternate function on PB6 (PWM0)
// Inputs: period is the 16-bit number of PWM clock cycles in one period (3<=period),
// duty is number of PWM clock cycles output is high  (2<=duty<=period-1)
// Output: none
void PWM0_Init(uint16_t period, uint16_t duty);
