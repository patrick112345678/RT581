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


ZBOSS Thermostat Cluster Test setup
=====================================

This application setup demonstrates Thermostat Cluster usage.
Setup contains two applications:

  - Zigbee Coordinator (which implements Thermostat cluster client role)
  - Zigbee Router (which implements Thermostat cluster server role)

These applications implements Zigbee 3.0 specification, Base Device Behavior specification and Zigbee Cluster Library 7 revision specification.
By default, both devices work on the 0 page 21 channel.


Setup structure
----------------

 - Makefile
 - thermostat_zc - *Zigbee Coordinator application with Thermostat Cluster (c)*
   - thermostat_zc.c
   - thermostat_zc.h
 - thermostat_zr - *Zigbee Router application with Thermostat Cluster (s)*
   - thermostat_zr.c
   - thermostat_zr.h
 - readme.txt - *This file*



Zigbee Coordinator (with Thermostat (c)) application
-----------------------------------------------

Zigbee Coordinator includes following ZCL clusters:

 - Basic (s)
 - Identify (s/c)
 - Thermostat (c)



Zigbee Router (with Thermostat (s)) application
----------------------------------------------

Zigbee Router includes following ZCL clusters:

 - Basic (s)
 - Identify (s)
 - Scenes (s)
 - Groups (s)
 - Thermostat (s)
 - Fan Control (s)
 - Thermostat UI (s)

Test scenario is defined in this application.


Setup behavior
---------------

- Zigbee Coordinator (with Thermostat (c) clister) creates network on the 21 channel
- Zigbee Router joins Zigbee Coordinator using the BDB commissioning
- Zigbee Coordinator starts sending Thermostat cluster commands:
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
   1. ZC - thermostat_zc
   2. ZR - thermostat_zr

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
    Item 1. ZC start.
    Item 2. ZR start.

 Expected outcome:
    For 'Test procedure' item 1:
        1.1. ZC -> Broadcast: Beacon Request
    For 'Test procedure' item 2:
        2.2. ZC -> Broadcast: Beacon
        2.3. ZR -> ZC: Association
        2.4. ZR -> ZC: BDB commissioning
        2.5. ZC -> ZR: Identify Query
        2.6. ZR -> ZC: Identify Query Response
        2.7. ZC -> ZR: Simple Descriptor Request
        2.8. ZR -> ZC: Simple Descriptor Response
        2.9. ZC -> ZR: Setpoint Raise/Lower
        2.10. ZR -> ZC: Default Response
              thermostat_zc generate a random value for the 'Amount' field
              of the 'Setpoint Raise/Lower' command, so we can receive this
              response too.
        2.11. ZC -> ZR: Set Weekly Schedule
        2.12. ZR -> ZC: Default Response
        2.13. ZC -> ZR: Get Weekly Schedule
        2.14. ZR -> ZC: Get Weekly Schedule Response
        2.15. ZC -> ZR: Clear Weekly Schedule
        2.16. ZR -> ZC: Default Response
        2.17. ZC -> ZR: Get Relay Status Log
        2.18. ZR -> ZC: Default Response
