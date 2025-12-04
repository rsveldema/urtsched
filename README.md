Library for user-space task scheduling
===========================================


We have 2 types of tasks:
- periodic
- idle tasks

Periodics are scheduled at a given future time and have a read-time deadline.
Idle tasks are scheduled as long as the next periodic deadline is still in the future.


We ensure real time behavior by:
- reserving a few cores for our real time tasks
- not allowing blocking tasks, we ensure real-time behaviour
   --> ** No task is allowed to perform blocking actions **
- all I/O should be performed async and use idle tasks to check if I/O has finished.

