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
Then join the bulb

Light Control application:
===========================
Light Control joins the ZC/ZR and searches any device with On/Off or Level Control cluster.

If HW supports buttons, the device has two buttons:
- the "SW2" button is a ON/UP button,
- the "SW1" button is a OFF/DOWN button.
The device behavior depends on the button's pressed time:
If the button is pressed and released less than 1 second, On (Off) command will be send to the bulb.
If the button is pressed more than 1 second, Step (Up or Down) command will be continiously send to the bulb until the button is released.
If HW supports LEDs, the white LED is used for the user notification, which button is being pressed (LED is on for ON/UP button, off for OFF/DOWN button).

LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).

If the device has two buttons:
- the "Button 1"  sends On command,
- the "Button 2"  sends OFF command,

If HW does not support buttons - once Dimmable Light is discovered, Light Control starts periodical On-Off-On-Off-...
command sending with 15 sec timeout.
