# Flapjack

This is a toy shell developed to learn about operating systems and the C programming language  
Flapjack is the name given to the shell while the language it runs is called Varelse  
Think of it as the commands are Varelse while the environment Varelse is run in is called Flapjack  

# References

- https://cloudaffle.com/series/customizing-the-prompt/moving-the-cursor/ for ansi codes for moving the cursor
- https://viewsourcecode.org/snaptoken/kilo/ for entering raw mode and handling user input in raw mode

# Commands
You do not know Varelse  
The language itself is designed to look alien  
## 1 2 ;
Move contents of register 2 into 1
## 1 "Data" :
Load the value "Data" into register 1
## -
Print contents of all the registers
## "Name" <
Declare a label called "Name"
## 1 >
Jump to label referenced by register 1
## 1 @
Change to directory referenced by register 1
## 1 2 ... #
Call program referenced by register 1 with args specified in the following registers given
## 1 2 ... _
Perform dir command with arguments specified in registers given
## )
Perform clear command
## 1 ... \
Perform echo command with arguments specified in registers given
## 1 (
Set stdin to what's in register 1
## 1 ]
Set stdout to what's in register 1
## }
Toggle stdout write / append mode

