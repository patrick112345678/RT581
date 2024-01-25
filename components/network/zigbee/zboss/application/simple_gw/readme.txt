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


ZBOSS Simple Gateway application
================================

This application is a Zigbee Coordinator device which is implemented using BDB API.
Default operational channel is set by SIMPLE_GW_CHANNEL_MASK.
Maximum operational devices number is 20.

Simple Gateway application includes following ZCL clusters:

  - On/Off Switch Config (s)
  - Identify (s/c)
  - Basic (s)
  - On/Off (c)
  - Scenes (c)
  - Groups (c)
  - IAS Zone (c)


The application folder structure
--------------------------------

- ias_cie_addon.c - *Additions for work with IAS Zone device (with IAS Zone (s) cluster)*
- ias_cie_addon.h - *Additions for work with IAS Zone device (with IAS Zone (s) cluster)*
- open-pcap.sh - *The script for openning logs*
- readme.txt - *This file*
- runng.sh - *The script for running this application on Network Simulator*
- simple_gw.c - *Simple Gateway application*
- simple_gw.h - *Simple Gateway application header file*
- simple_gw_device.h - *Simple Gateway application header file, which defines Simple Gateway used clusters*


Application behavior
---------------------

Upload a Simple Gateway binary to your Zigbee device. After powering it on the first time,
a Simple GW forms a ZB network. After power restarting the device, existing ZB network
parameters are used, the same network exists (if NVRAM erasing at start is not configured).
On start up the Simple Gateway opens the network for 180 sec. During this time any device joining
is allowed. In order to join your ZB device (bulb/smart plug/etc), power it on during this time
frame - the device will associate the network automatically.

After the successful association, the Simple GW applications starts discovering On/Off Server
cluster. If it is found, Simple GW configures bindings and reporting for the device and
then then it starts to send On/Off Toggle commands periodically. Timeout between
consecutive commands sending is calculated using the following formula:
T = (device_index * 5) sec.
device_index is a number of joined device: 1, 2, ..., 20.


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - simple_gw
   2. ZR - bulb

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
   For 'Test procedure' item 2:
     2.1. ZC -> Broadcast: Beacon Request
     2.2. ZC -> Broadcast: Beacon
     2.3. ZR -> ZC: Association
     2.4. ZR -> ZC: BDB commissioning
     2.5 ZC -> ZR: Match Descriptor Request
     2.6. ZR -> ZC: Match Descriptor Response
     2.7. ZC -> ZR: Extended Address Request
     2.8. ZR -> ZC: Extended Address Response
     2.9. ZC -> ZR: Bind Request
     2.10. ZR -> ZC: Bind Response
     2.11. ZC -> ZR: Configure Reporting
     2.12. ZR -> ZC: Configure Reporting Response
     2.13. ZC -> ZR: Toggle. ZR -> ZC: Default Response (x2)
     2.14. ZR -> ZC: Report Attribute. ZC -> ZR: Default Response (x2)
   For 'Test procedure' item 4:
     4.1. ZC -> ZR: Toggle. ZR -> ZC: Default Response (x2)
     4.2. ZR -> ZC: Report Attribute. ZC -> ZR: Default Response (x2)
   For 'Test procedure' item 6:
     6.1. ZC -> Broadcast: Permit Join Request
     6.2. (ZC) -> ZR -> Broadcast: Permit Join Request
     6.3. ZC -> ZR: Toggle. ZR -> ZC: Default Response (x2)
     6.4. ZR -> ZC: Report Attribute. ZC -> ZR: Default Response (x2)
     6.5. Check period for Attributes Reporting
