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



ZBOSS Smart Plug application
============================================================================

Smart Plug application implements Zigbee 3.0 specification and Base Device Behavior (ZR).
Smart Plug application may be tested with Simple Gateway (ZC) application.

Smart Plug includes the following ZCL clusters:

  1. Basic (s)
  2. Identify (s)
  3. Metering (s)
  4. On/Off (s)
  5. OTA FW upgrade (c)

The application structure
---------------------------

  - include

    - sp_device.h
    - sp_hal_stub.h
    - sp_config_stub.h

  - sp_device

    - sp_device.c
    - sp_hal_stub.c
    - sp_ota_osif.c

  - readme.txt - *This file*

Smart Plug application behavior (operation states)
-----------------------------------------------------

1. Commissioning mode:
After the power on, device checks the network state. If it was not previously joined Zigbee network, it performs one
commissioning attempt. If an open Zigbee network found (received beacon with the Association Permit flag set to 1),
smart plug joins this network. If no open Zigbee network found, device goes to the idle mode.
If device was previously joined, it starts normal operation mode immediately after power on.
To start commissioning manually hold the button for 2 seconds when device is in the idle mode.

2. Normal mode:
After join and successful startup configuration (bindings/reporting/etc) device switches to the normal operation mode.
In this mode it accepts On/Off commands, sends reporting for Metering data etc. Relay LED indicates relay On/Off state.

3. Idle mode:
In idle mode device does not operating with Zigbee. Possible operations:

  - manual Relay control
  - initiate commissioning

4. Reset to Factory defaults:
Device switches to this mode on ZCL Reset to Factory Defaults command from coordinator, or manually (using the button).
Smart Plug leaves the network (if it is joined) and goes to the idle mode.


Smart Plug controls/indication when running on hardware
---------------------------------------------------------

- Button (if supported)
-- Relay On/Off (single press)
-- Manual reset to factory defaults (power off device, press the button, power on device and
   hold the button for 5 seconds)
-- Manual commissioning start (hold the button for 2 seconds when device is in idle mode)
- Operation LED (if supported)
-- Indicates current operation mode
- Relay LED (if supported)
-- Indicates relay On/Off state


### Smart Plug operational LED blinking modes

- Commissioning: blink periodically 1 second on, 1 second off
- Normal operation: blink periodically 300 ms on, 3 sec off
- Idle: 3 short blinks (300 ms on, 300 ms off), 3 sec off
- Reset to factory defaults: 200 ms on, 200 ms off 10 times


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - simple_gw
   2. ZR - sp_device

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   Item 1. ZC start.
   Item 2. ZR start.
   Item 3. Turn ZR off
   Item 4. Turn ZR on
   Item 5. Turn ZC off
   Item 6. Turn ZC on

 Expected outcome:
   For 'Test procedure' item 1:
     1.1. ZC -> Broadcast: Beacon Request
   For 'Test procedure' item 2:
     2.2. ZC -> Broadcast: Beacon
     2.3. ZR -> ZC: Association
     2.4. ZR -> ZC: BDB commissioning
     2.5. ZC -> ZR: Match Descriptor Request
     2.6. ZR -> ZC: Match Descriptor Response
     2.7. ZC -> ZR: Extended Address Request
     2.8. ZR -> ZC: Extended Address Response
     2.9. ZC -> ZR: Bind Request
     2.10. ZR -> ZC: Bind Response
     2.11. ZC -> ZR: Configure Reporting
     2.12. ZR -> ZC: Configure Reporting Response
     2.13. ZC -> ZR: Toggle (x2)
     2.14. ZR -> ZC: Default Response
     2.15. ZR -> ZC: Report Attribute (x3)
   For 'Test procedure' item 4:
     4.1. ZR -> ZC: Report Attribute (x3)
     4.2. ZC -> ZR: Toggle (x2)
   For 'Test procedure' item 6:
     6.1. ZC -> Broadcast: Permit Join Request
     6.2. (ZC ->) ZR -> Broadcast: Permit Join Request
     6.3. ZC -> ZR: Toggle (x2)
     6.4. ZR -> ZC: Report Attribute (x3)
