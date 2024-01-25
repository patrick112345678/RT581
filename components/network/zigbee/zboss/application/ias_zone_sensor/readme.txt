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


ZBOSS IAS Zone Sensor application (IZS)
============================================================================

IZS application implements Zigbee 3.0 specification and Base device behavior (end device, ED). Generic IAS Zone device
implementation is suitable for reuse while any IAS Zone device implementation (smoke detector, door open/close, etc.)
IZS application may be tested with simple_gw ZC.

IZS includes the following ZCL clusters
Mandatory for all the roles:

  - Basic (s)
  - Identify (s)
  - IAS Zone (s)
  - OTA FW upgrade (c)

Following clusters are supported for battery powered ED:

  - Poll control (s)
  - Power Configuration (s)

The application folder structure
----------------------------------

  - readme.txt
  - inc

    - izs_avg_hal.h
    - izs_config.h
    - izs_device.h
    - izs_hal.h

  - src

    - izs_avg_val.c
    - izs_battery.c
    - izs_device.c
    - izs_device_utils.c
    - izs_hal_stub.c
    - izs_nvram.c - *Application-defined NVRAM logic*
    - izs_ota_osif.c
    - izs_start.c - *IAS Zone initial routine*

ZBOSS IAS Zone Sensor application behavior
-------------------------------------------

### Startup

On starting up, IZS application does association attempt if the device was not associated to any network yet
or it starts rejoining, if it was joined to a network before.
IZS application implements a set of states that may be used to indicate device status change:

  - Association or rejoining is in progress
  - Association is finished (success or fail)
  - Rejoining is finished
  - etc

These states may be used to provide LED indication or some other kind of user indication.


### Production configuration data

Production configuration data (PCD) may be used to specify a device parameters that may be changed in production
time to produce different flavors of a device. The following parameters must be specified as a part of PCD:

  1. Manufacturer name (will be copied to Basic cluster attribute)
  2. Model Id (will be copied to Basic cluster attribute)

PCD is read and applied on application start time (before ZDO is started).
For tools and readmes on generating PCD refer to production_config_generator


### ZB association

IZS joins a target network while 16 other open networks are running (ZB3.0, HA or SE). Background: if a network is
not suitable for some reason, it will be blacklisted and its beacons will be filtered out during association.
If association was unsuccessful, IZS sleeps for (N * 5) seconds and then continue association attempts. N starts with 1
and enlarges with each attempt.

### Rejoining

IZS supports rejoin back-off procedure. On detecting a communication link failure with its parent, IZS starts rejoin
back-off procedure.


### Reset to factory defaults

IZS supports reset to FD in the following cases:

  1. Device leaves a network
  2. Receives a Reset to FD command
  3. Action is initiated by a user. How to trigger a reset to FD by user action is out of scope for IZS.


### Enrollment procedure

1. Support Auto-pair scenario
2. Support Trip-to-pair scenario


### Notifications sending

Event queueing algorithm for notification sending is supported.


### Power configuration cluster

1. Attributes for 1 battery pack are supported.
2. Supported battery threshold: 60 days.
3. ZoneStatus attribute will be updated if battery defect or low battery is detected.


### OTA FW upgrade cluster

IZS supports ZCL OTA upgrade.

### Fast polling

Fast polling is independent of the poll-control cluster short and/or long poll interval.
Existing ZBOSS adaptive poling is enabled.


### Detector support

Detector control API (called by the main app):

  1. Detector HW init
  2. Detector HW enable
  3. Detector HW disable

API function izs_update_ias_zone_status() supports the status change events:

  1. Alarm status set/clear
  2. Temper status set/clear
  3. Defect status set/clear
  4. Test mode status set/clear
  5. Low battery stats set/clear
  6. Battery defect set/clear
  7. Battery 60 days threshold set/clear

This function may be called by the detector hw handler module.


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - simple_gw
   2. ZED - izs_device

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   1. ZC start.
   2. ZED start.
   3. Turn ZED off
   4. Turn ZED on
   5. Turn ZC off.
   6. Turn ZC on.

 Expected outcome:
   For 'Test procedure' item 2:
     2.1. ZC -> Broadcast: Beacon Request
     2.2. ZC -> Broadcast: Beacon
     2.3. ZED -> ZC: Association
     2.4. ZED -> ZC: BDB commissioning
     2.5. ZED -> ZC: Match Descriptor Request
     2.6. ZC -> ZED: Match Descriptor Response
     2.7. ZC -> ZED: Match Descriptor Request
     2.8. ZED -> ZC: Match Descriptor Response
     2.9. ZED -> ZC: Zone Enroll Request
     2.10. ZC -> ZED: Zone Enroll Response
     2.11. ZC -> ZED: Write Attributes
     2.12. ZED -> ZC: Write Attributes Response
     2.13. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0021
     2.14. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0020
   For 'Test procedure' item 4:
     4.1. ZED -> ZC: Rejoin
     4.5. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0021
     4.6. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0020
   For 'Test procedure' item 6:
     6.1. ZC -> Broadcast: Permit Join Request
     6.2. ZC -> Broadcast: Parent Announce
     6.3. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0021
     6.4. ZED -> ZC: Zone Status Change Notification Zone Status: 0x0020
