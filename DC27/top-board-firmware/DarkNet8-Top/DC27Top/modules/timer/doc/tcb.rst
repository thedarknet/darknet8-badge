======================
TCB driver
======================
The capabilities of the 16-bit Timer/Counter type B (TCB) include frequency and waveform generation, and input capture with time and frequency measurement of digital signals. It consists of a base counter and control logic which can be set in one of eight different modes, each mode providing unique functionality. The base counter is clocked by the peripheral clock with optional prescaling.

Features
--------
* Initialization

Applications
------------
* Capture the event input.
* Periodic Interrupt
* Timeout Checking
* 8-bit Pulse Width Modulation
* Single-shot pull-up high

Dependencies
------------
* CLKCTRL for clock
* CPUINT for Interrupt
* EVSYS for events
* UPDI for debug

Note
----
* ISR will be generated only when Global Interrupt checkbox and driver_isr Harness checkbox are enabled

Concurrency
-----------
N/A

Limitations
-----------
N/A

Knows issues and workarounds
----------------------------
N/A

