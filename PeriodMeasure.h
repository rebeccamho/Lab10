//#PeriodMeasure.h
//Matt Owens and Rebecca Ho
//April 11, 2017

//------------PeriodMeasure_Init------------
// Initializes PB7, Timer0B, and Timer1A
// Input: the desired reload value for Timer1A
// Output: none
void PeriodMeasure_Init(uint32_t);

//------------CheckIfNewSpeed------------
// Checks if there is a new speed value and assigns it to
// the pointer if there is
// Input: none
// Output: true if a new value was assigned, false otherwise
bool CheckIfNewSpeed(uint32_t* speed,uint16_t* duty,uint32_t* desired_speed);

void SetDesiredSpeed(uint32_t);

void SetDuty(uint32_t);
