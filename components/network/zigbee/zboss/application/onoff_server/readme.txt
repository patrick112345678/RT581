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



ZBOSS OnOff Cluster set
==========================

This set of applications demonstrates user defined Custom Cluster implementation.
The set contains two applications:

  - Zigbee Coordinator (which acts as On/Off output)
  - Zigbee End Device (which acts as On/Off switch)

These applications implements Zigbee 3.0 specification, Base Device Behavior specification and Zigbee Cluster Library 7 revision specification.
By default, both devices work on the 0 page 21 channel.
Devices can be compiled with installcode check(for SE build). By default it is disabled.
In case of compiling these devices for HW, they can be compiled with enabled buttons and leds or with disabled ones, depending on the vendor header used. By default leds and buttons are usually disabled.

The application set structure
------------------------------

 - Makefile
 - on_off_output_zc.c - *On/Off output coordinator application*
 - on_off_switch_zed.c - *On/Off switch end device application*
 - readme.txt - *This file*
 - runng.sh - *Script for running setup on Network Simulator*
 - open-pcap.sh - *Script for openning .pcap files*


Zigbee Coordinator (On/Off output) application
-----------------------------------------------

Zigbee Coordinator includes following ZCL clusters:

 - Basic (s)
 - Identify (s)
 - OnOff (s)
 - Scenes (s)
 - Groups (s)

Zigbee End Device (On/Off switch) application
----------------------------------------------

Zigbee End Device includes following ZCL clusters:

 - Basic (s)
 - Identify (s/c)
 - OnOff Switch Config (s)
 - OnOff (c)
 - Scenes (c)
 - Groups (c)

This application also includes an example of the user defined ZBOSS main loop.


Applications behavior
---------------------

- Zigbee Coordinator creates network on the 21 channel
- Zigbee End Device joins Zigbee Coordinator using the BDB commissioning
  - Zigbee Coordinator sends Simple Descriptor Request and Zigbee Router replies with the Simple Descriptor Response
  - Zigbee Coordinator saves the endpoint and the short address of the connected device in 'finding_binding_cb'
- Zigbee End Device sends Permit Join request with permit duration 0 to close the network
- Zigbee End Device starts sending requests of different descriptors to the Zigbee Coordinator (just as an example of the descriptors API usage):
  - Node Descriptor request
  - Power Descriptor request
  - Simple Descriptor request
  - Active Endpoint request
  - System Server Discovery request
- Zigbee End Device (On/Off switch) sends ZCL On/Off Toggle commmand to the Zigbee Coordintor (On/Off output) via BDB binding every 7 sec. If application runs on hardware, Toggle command can be trigged by pressing button.


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - on_off_output_zc
   2. ZED - on_off_switch_zed

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   Item 1. Turn ZC on.
   Item 2. Turn ZED on
   Item 3. Turn ZED off
   Item 4. Turn ZED on
   Item 5. Turn ZC off. Wait 1 minute.
   Item 6. Turn ZC on. Wait 1 minute.

 Expected outcome:
    For 'Test procedure' item 1:
      1.1. ZC -> Broadcast: Beacon Request
    For 'Test procedure' item 2:
      2.1. ZC -> Broadcast: Beacon
      2.2. ZED -> ZC: Association
      2.3. ZED -> ZC: BDB commissioning
      2.4. ZED -> ZC: Simple Descriptor Request
      2.5. ZC -> ZED: Simple Descriptor Response
      2.6. ZED -> ZC: Toggle. ZC -> ZED: Default Response. (x5)
    For 'Test procedure' item 4:
      4.1. ZED -> ZC: rejoin
      4.2. ZED -> ZC: Toggle. ZC -> ZED: Default Response. (x5)
    For 'Test procedure' item 6:
      6.1. ZED -> ZC: rejoin
      6.2. ZC -> Broadcast: Parent Announce
      6.3. ZED -> ZC: Toggle. ZC -> ZED: Default Response. (x5)
