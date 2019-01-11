# cmput-379-sdn
CMPUT 379 assignment 2 and 3

## Description
A c++ program that implements the transactions performed by a simple linear software-defined 
network (SDN). 

### Objectives
This programming assignment is intended to give experience in
developing peer-to-peer programs that utilize signals for examining the
progress of the running processes, FIFOs and Sockets for communication,
and I/O multiplexing for nonblocking I/O.

### Compiling the program
Makefile commands (assignment spec):
  - Executing ‘make’ produces the a3sdn executable file.
  - Executing ‘make clean’ removes unneeded files produced in compilation.
  - Executing ‘make tar’ produces the ‘submit.tar’ archive.

### Controller Mode
The program can be invoked as a controller using  
  ```a3sdn cont nSwitch port```  
where cont is a reserved word, and nSwitch specifies the
number of switches in the network (at most MAX NSW= 7 switches).

### Switch Mode
The program can also be invoked as a switch using:  
    ```a3sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhigh serverAddress port```  
In this form, the program simulates switch swi by processing traffic read
from file trafficFile. Port 1 and port 2 of swi are connected to switches
swj and swk, respectively. Either, or both, of these two switches may be null.
Switch swi handles traffic from hosts in the IP range [IPlow-IPhigh].
Each IP address is ≤ MAXIP (= 1000) and >= 0.
Sequence matters. The controller should be set up first and should not be
closed before the switches.
