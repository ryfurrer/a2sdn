# cmput-379-sdn
CMPUT 379 assignment 2 and 3

## Description
A c++ program that implements the transactions performed by a simple linear software-defined 
network (SDN). 

### Controller Mode
The program can be invoked as a controller using `a3sdn cont nSwitch` where `cont`
is a reserved word, and `nSwitch` specifies the number of switches in the network 
(at `mostMAXNSW=7` switches). 

### Switch Mode
The program can also be invoked as a switch using 
`a3sdn swi trafficFile [null|swj] [null|swk] IPlow-IPhigh`. In this form, the 
program simulates switch `swi` by processing traffic read from file `trafficFile`.
Port 1 and port 2 of `swi` are connected to switches `swj` and `swk`, respectively. 
Either, or both, of these two switches may be `null`. Switch `swi` handles traffic
fromhosts in the IP range `[IPlow-IPhigh]`. 

### Other Requirements
Each IP address is `≤MAXIP(= 1000)`.

Data transmissions among the switches uses fifos while communication between switches 
and controllers was done via TCP sockets.

Also the requirement for switches to disconnect and have the SDN still operate “effectively” was added.

Each fifo connection is named `fifo-x-y` where `x=/=y` and `x,y∈[1,MAXNSW]` for a switch.
