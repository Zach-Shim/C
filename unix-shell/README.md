# Unix-Shell
## Description
Unix shell developed in C that can handle basic user commands through separate processes.  
Supports input and output redirection, as well as direct interprocess communication between two processes using a pipe.  

## What I Learned
Developed further understanding of operating system functions and concepts such as IPC, parent-child relationships, and process lifecycle.  
Implemented using fork(), pipe(), open(), close(), and dup2() system calls.  

## Capabilities
Can handle any basic command (i.e., ls, ls -l, ls -l wc, etc.).  
Can handle redirection operators (i.e., ls > out.txt, cat < out.txt, etc.) (cannot handle multiple chaining).  
Can handle pipe calls (i.e. ls -l | wc, grep | shimz@uw.edu | wc -l, etc.).  
