
  RFP_Proj2
---------------

  Hardware:
     - Ready for PIC board (see Docs/RFP_Setup.JPG)
     - PIC18LF26K22 processor

  Demonstrates 2 things:
     1.  Setting up a 1ms timer
     2.  Setting up the timer on an Interrupt


  The 1ms timer (see timer.h) is initialized for each of the 2 clocks from Proj1:
        - 64Mhz Internal
        - 32Mhz External

  1ms timer interrupt can be either Low Priority or High Priority depending on
  TMR0IP setting (see PICRegs_Init_TMR0_Intrs)

  Code in main demonstrates the use of TIM_GetDeltaB() and TIM_GetDeltaW() in order
  to achieve a 20ms timer and a 1sec timer respectively.

  Scope output for the 20ms timer is on RB1, and scope output for the 1sec timer
  is on port RC1.  See Docs/scope_3.png for Scope image.

