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


ZBOSS Scenes Cluster Test
================================

This set of applications demonstrates Scenes Cluster usage.
The set contains two applications:

  - Zigbee Coordinator (which acts as On/Off output with Scenes server)
  - Zigbee End Device (which acts as device with Scenes client)

These applications implements Zigbee 3.0 specification, Base Device Behavior specification and Zigbee Cluster Library 7 revision specification.
By default, both devices work on the 0 page 21 channel.


The application set structure
------------------------------

 - Makefile
 - scenes_zc.c - *On/Off output coordinator application*
 - zcenes_zed.c - *Scenes client end device application*
 - readme.txt - *This file*
 - run.sh - *Script for running setup on Network Simulator*
 - test_output_dev.h -  *Coordinator application header file*
 - test_switch_dev.h - *End Device application header file*
 - scenes_test.h - *A header file with common definitions for testing Scenes (s) cluster*


Zigbee Coordinator (On/Off output with Scenes server) application
------------------------------------------------------------------

Zigbee Coordinator includes following ZCL clusters:

 - Basic (s)
 - Identify (s)
 - OnOff (s)
 - Scenes (s)
 - Groups (s)

This application also defines and uses its own NVRAM storage (application-specific NVRAM), if NVRAM feature has been enabled in the ZBOSS build.
Scenes commands are processed in the application. See registered by the ZB_ZCL_REGISTER_DEVICE_CB callback their implementation.

Zigbee End Device (On/Off switch) application
----------------------------------------------

Zigbee End Device includes following ZCL clusters:

 - Basic (s)
 - Identify (s/c)
 - OnOff Switch Config (s)
 - OnOff (c)
 - Scenes (c)
 - Groups (c)

Test scenario is defined in this application.


Applications behavior
---------------------

- Zigbee Coordinator creates network on the 21 channel
- Zigbee End Device joins Zigbee Coordinator using the BDB commissioning
- Zigbee End Device starts sending requests of different descriptors to the Zigbee Coordinator (just as an example of the descriptors API usage):
  - Add Scene
  - View Scene
  - Remove a Scene
  - Remove all Scenes
  - Store Scene
  - Recall Scene
  - Get Scene Membership


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - scenes_zc
   2. ZR - scenes_zed

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   Item 1. Turn ZC on.
   Item 2. Turn ZED on
   Item 3. Turn ZED off.
   Item 4. Turn ZED on.
   Item 5. Turn ZC off.
   Item 6. Turn ZC on.


 Expected outcome:
   For 'Test procedure' item 1:
     1.1. ZC -> Broadcast: Beacon Request
   For 'Test procedure' item 2:
     2.1. ZC -> Broadcast: Beacon
     2.2. ZED -> ZC: Association
     2.3. ZED -> ZC: BDB commissioning
     2.4. ZED -> ZC: Add Scene
     2.5. ZC -> ZED: Add Scene Response
     2.6. ZED -> ZC: View Scene
     2.7. ZC -> ZED: Command: View Scene Response
     2.8. ZED -> ZC: Remove a Scene
     2.9. ZC -> ZED: Remove a Scene Response
     2.10. ZED -> ZC: Remove all Scenes
     2.11. ZC -> ZED: Remove all Scene Response
     2.12. ZED -> ZC: Store Scene
     2.13. ZC -> ZED: Store Scene Response
     2.14. ZED -> ZC: Recall Scene
     2.15. ZC -> ZED: Default Response
     2.16. ZED -> ZC: Get Scene Membership
     2.17. ZC -> ZED: Get Scene Membership Response
   For 'Test procedure' item 4:
     4.1. ZED -> ZC: Rejoin
     4.2. ZED and ZC send scenes cluster commands
   For 'Test procedure' item 6:
     6.1. ZC -> Broadcast: Permit Join Request
     6.2. ZC -> Broadcast: Parent Announce
     6.3. ZED -> ZC: Rejoin
