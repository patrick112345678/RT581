Demo setup:
===========
The demo should be set in the following way:
1. Flash each device individually and power-off after programming.
2. Power on Light Coordinator.
3. Power on Dimmable Light device.
4. Power on Light Control device.

For the detailed information of the each device, refer to the sections below.

Light Coordinator application:
================================
Light Coordinator device establishes and joins devices to the Zigbee network.
LED1 indicates the programm start.

Dimmable Light application:
===========================
Dimmable Light joins the ZC and waits for a Light Control to join the network.
Both On/Off and Level Control clusters are supported.
Device state and brightness level are store in Non-Volatile Memory, so bulb's parameters are restored after power cycle.
LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).
LED3 indicates the bulb state. Lights when On/Off cluster On command received.


Light Control application:
===========================
Light Control joins the ZC/ZR and searches any device with On/Off or Level Control cluster.

LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).

The device has two buttons:
- the "Button 1"  sends On command,
- the "Button 2"  sends OFF command,

If HW does not support buttons - once Dimmable Light is discovered, Light Control starts periodical On-Off-On-Off-...
command sending with 15 sec timeout.
