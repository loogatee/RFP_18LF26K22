
  RFP_Proj1
------------------

  Hardware:
     - Ready for PIC board (see Docs/RFP_Setup.JPG)
     - PIC18LF26K22 processor


  Setup the Oscillator 2 different ways:
       - Internal
       - External

  Verify correct operation using scope


  Internal Oscillator can be maxed out at 16Mhz.  Enabling the 4xPLL gives a Clock frequency
  of 64Mhz.

  There is an 8Mhz External Oscillator on the board.   Enabling the 4xPLL gives a frequency
  of 32Mhz.   That's the Max for the 26K22 using an 8Mhz crystal.

  I'm using pin C1 to output a signal that will show the effective rate at which instructions
  are executed.   The important thing to know is that 1 instruction executes every 4 clock
  cycles.

            Instruction_Freq = Clock_Freq / 4
                 or
            Clock_Freq = Instruction_Freq * 4

  See Scope output:
     - Docs/C1_Internal_Osc_64Mhz.png   shows freq of 8Mhz.  1/2 is 16Mhz
     - Docs/C1_External_Osc_32Mhz.png   shows freq of 4Mhz.  1/2 is 8Mhz

  Both images show the frequency for an entire period.   Half of that equals 1 instruction.

  





