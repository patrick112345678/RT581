    ZBOSS Zigbee software protocol stack

    Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
    www.dsr-zboss.com
    www.dsr-corporation.com
    All rights reserved.

    This is unpublished proprietary source code of DSR Corporation
    The copyright notice does not evidence any actual or intended
    publication of such source code.

    ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
    Corporation

    Commercial Usage
    Licensees holding valid DSR Commercial licenses may use
    this file in accordance with the DSR Commercial License
    Agreement provided with the Software or, alternatively, in accordance
    with the terms contained in a written agreement between you and
    DSR.



ZBOSS Light Sample
=========================

The demo should be set in the following way:

  1. Flash each device individually and power-off after programming.
  2. Power on Light Coordinator.
  3. Power on Dimmable Light device.
  4. Power on Light Control device.

For the detailed information of the each device, refer to the sections below.

The application set structure
-----------------------------------

  - Makefile
  - dimmable_light

    - bulb.c
    - bulb.h
    - bulb_hal.c
    - bulb_hal.h
    - zb_ha_bulb.h

  - light_control

    - light_control.c
    - light_control.h
    - light_control_hal.c
    - light_control_hal.h

  - light_cordinator

    - light_zc.c
    - light_zc.h
    - light_zc_hal.c
    - light_zc_hal.h

  - readme.txt - *This file*


Light Coordinator application
--------------------------------

Light Coordinator device establishes and joins devices to the Zigbee network.
LED1 indicates the programm start.

Dimmable Light application
------------------------------

Dimmable Light joins the ZC and waits for a Light Control to join the network.
Both On/Off and Level Control clusters are supported.
Device state and brightness level are store in Non-Volatile Memory, so bulb's parameters are restored after power cycle.
If HW supports LEDs, level control is obtained by Green LED connected to the PWM channel.
LED1 indicates the programm start.
LED2 indicates comissioning status (ON when joined the network).
LED3 indicates the bulb state. Lights when On/Off cluster On command received.


Light Control application
---------------------------------

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


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
    1. ZC - light_coordinator
    2. ZR - bulb
    3. ZED - light_control

Initial conditions:
    1. All devices are factory new and powered off until used.

 Test procedure:
    1. Power on ZC.
    2. Power on ZR.
    3. Turn ZED on.
    4. Turn ZC off.
    5. Turn ZC on.
    6. Turn ZR off.
    7. Turn ZR on.
    8. Turn ZED on/off.

 Expected outcome:
    For 'Test procedure' item 2:
      2.1. ZC -> Broadcast: Beacon request
      2.2. ZC -> Broadcast: Beacon
      2.3. ZR -> ZC: association
      2.4. ZR -> ZC: BDB commissioning
      2.5. ZC -> ZR: Match Descriptor Request for input clusters
      2.6. ZR -> ZC: Match Descriptor Response
    For 'Test procedure' item 3:
      3.1. ZED -> ZC: association
      3.2. ZED -> ZC: BDB commissioning
      3.3. ZC -> ZED: Match Descriptor Request for input clusters
      3.4. ZED -> ZC: Match Descriptor Response
      3.5. ZC -> ZED: Match Descriptor Request for output clusters
      3.6. ZED -> ZC: Match Descriptor Response
      3.7. ZC -> ZED: Bind Request for OnOff cluster
      3.8. ZED -> ZC: Bind Response with Success
      3.9. ZC -> ZED: Bind Request for Level Control cluster
      3.10. ZED -> ZC: Bind Response with Success. ZED power cycle
      3.11. ZED -> ZR: send OnOff commands
    For 'Test procedure' item 4:
      4.1. ZED -> ZR: Rejoin
    For 'Test procedure' item 5:
      5.1. ZC -> Broadcast: Permit Join Request
      5.2. (ZC ->) ZR -> Broadcast: Permit Join Request
      5.3. ZC -> Broadcast: Parent Announce
      5.4. (ZC ->) ZR -> Broadcast: Parent Announce
      5.5. ZR -> ZC: Parent Announce Response
    For 'Test procedure' item 6:
      6.1 ZED -> ZR: send OnOff commands
      6.2. ZED -> ZC: Rejoin
      6.3. ZC -> ZED: Match Descriptor Request for Input clusters
      6.4. ZED -> ZC: Match Descriptor Response
      6.5. ZC -> ZED: Match Descriptor Request for output clusters
      6.6. ZED -> ZC: Match Descriptor Response
    For 'Test procedure' item 7:
      7.1 ZED -> ZR: send OnOff commands
      7.2 Route request ZR -> Broadcast
      7.3 Route reply ZC -> ZR
    For 'Test procedure' item 8:
      8.1. ZED -> ZC: Rejoin
      8.2. ZC -> ZED: Match Descriptor Request for output clusters (use if ZED rejoins to ZC)
      8.3. ZED -> ZC: Match Descriptor Response (use if ZED rejoins to ZC)
      8.4. ZED -> ZR: send OnOff commands
