# OS161 Kernel development and extension

## About

This is a project that involved implementing various portions of an operating system. The main goal was to gain a strengthened understanding of concurrency, synchronization, virtual memory, system calls, file systems, device drivers, memory management, virtual memory, file systems, networking and security.

OS/161 is a teaching operating system, that is, a simplified system used for teaching undergraduate operating systems classes. It is BSD-like in feel and has more "reality" than most other teaching OSes; while it runs on a simulator it has the structure and design of a larger system.


## About System/161

System/161 is a machine simulator that provides a simplified but still realistic environment for OS hacking. It is a 32-bit MIPS system supporting up to 32 processors, with up to 31 hardware slots each holding a single simple device (disk, console, network, etc.) It was designed to support OS/161, with a balance of simplicity and realism chosen to make it maximally useful for teaching. However, it also has proven useful as a platform for rapid development of research kernel projects.

System/161 supports fully transparent debugging, via remote gdb into the simulator. It also provides transparent kernel profiling, statistical monitoring, event tracing (down to the level of individual machine instructions) and one can connect multiple running System/161 instances together into a network using a "hub" program.

---
The following features have been implemented by me in this project: <br>
1. <b>Implementation of Locks, Condition Variables and Reader-Writer Locks</b> <br>
2. <b>Implementation of file system calls - open, close, read, write, getpid, dup2</b> <br>
3. <b>Implementation of Process System calls - fork, execv, sbrk</b> <br>
4. <b>Implementation of a fully functional virtual memory subsystem - uses paging and swapping to manage memory</b> <br>
<br>