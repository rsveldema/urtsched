Library for application-level real-time task scheduling
===========================================

For real-time Linux applications, the options are currently limited.
We can rely on Linux's realtime signal delivery which isn't very flexible 
or easy to get right.

This library structures this a bit.
We have 2 types of tasks:
- periodic tasks
- idle tasks

Periodics are scheduled at a given at fixed intervals (say every 100 milliseconds).
Idle tasks are scheduled as long as the next periodic deadline is still in the future.

We ensure real time behavior by:
- reserving a few cores for our real time tasks
- not allowing blocking tasks, we ensure real-time behaviour
   --> ** No task is allowed to perform blocking actions **
- all I/O should be performed async and use idle tasks to check if I/O has finished.

