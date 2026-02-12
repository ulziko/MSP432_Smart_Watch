# MSP432 SMART WATCH 

> * Embedded Software for the Internet of Things class 2025 University of Trento*

**Functionalities:**
- Time Display 
- Alarm
- Step Counter
- Flappy Bird Game

## Architecture


<summary>Project Layout</summary>
> The project is designed to be built and ran on the MSP432 board with the educational booster pack peripherial plugged in.

## Build Requirements

<summary>Hardware Requirements</summary>

 MSP432 board.  
 
 The educational booster pack compatible with such board.  
 
 A computer capable of flashing binaries on the target device.  
 
 <summary>Software Requirements</summary>
 
 The code is distributed as a Code Composer Studio project so that ide shall be installed on the flashing device.  
 
 To build the code, the simple link msp432p4 sdk library must be provided and linked to the executable.  
 

## Development Notes

It is recommended to add entries in the handlers array inside the tasks.c file instead of implenting interrupt handlers when developing new tasks. This is necessary in order to avoid the redefinition of the interrupt handler headers.
