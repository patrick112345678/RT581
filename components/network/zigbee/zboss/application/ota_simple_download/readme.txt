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



OTA Upgrade Cluster - Simple Download (Upgrade)
==================================================

This set of applications demonstrates OTA Upgrade Cluster usage.
The set contains two applications:

  - Zigbee Coordinator (OTA Upgrade Server)
  - Zigbee Router (OTA Upgrade Scenes client)

These applications implements Zigbee 3.0 specification, Base Device Behavior specification and Zigbee Cluster Library 7 revision specification.
By default, both devices work on the 0 page 21 channel.

The application set structure
------------------------------

  - ota_upgrade_client.h - *Zigbee Router (OTA Upgrade Scenes client) header file*
  - ota_upgrade_server.h - *Zigbee Coordinator (OTA Upgrade Server) header file*
  - ota_upgrade_test_common.h - *Common definitions for the test*
  - ota_upgrade_zc.c - *Zigbee Coordinator (OTA Upgrade Server) application*
  - ota_upgrade_zr.c - *Zigbee Router (OTA Upgrade Scenes client) application*
  - readme.txt - *This file*

Zigbee Coordinator (OTA Upgrade Server) application
------------------------------------------------------------------

Zigbee Coordinator includes following ZCL clusters:

 - Basic (s)
 - OTA Upgrade (s)


Zigbee Router (OTA Upgrade Scenes client) application
----------------------------------------------

Zigbee End Device includes following ZCL clusters:

 - Basic (s)
 - OTA Upgrade (c)

Applications behavior
----------------------

- The ZC create Zigbee network on the 21 channel.
- The ZR connects to the network.
- The ZR initiates the process of searching the OTA upgrade server, by issuing a match descriptor request.
- The ZC sends OTA Image Notify command (20 sec after startup - it is scheduled in application), which informs the ZR about file with manufacturer ID and Image Type ID = ZR value.
- The ZR sends OTA Query Next Image request to the ZC with its manufacturer ID, Image Type ID, and current firmware version.
- The ZC sends OTA Query Next Image response with a status code of SUCCESS.
- The ZR starts the download process by issuing an OTA Image Block Request (Manufacturer ID, Image Type ID, firmware version from previous request. File offset = 0)  The maximum data size shall be set by the client
- The ZC sends OTA Image Block Response.
- The ZR sends OTA Upgrade End Request command.
- The ZC sends OTA Upgrade End Response command.


Test Script - SDK Samples
=============================
 Objective:
   Test and test scripts under SDK Samples context, performs high-level/functionality checks.

 Devices:
   1. ZC - ota_simple_download_zc
   2. ZR - ota_simple_download_zr

 Initial conditions:
   1. All devices are factory new and powered off until used.

 Test procedure:
   Item 1. Turn ZC on.
   Item 2. Turn ZR on.

 Expected outcome:
    For 'Test procedure' item 1:
      1.1. ZC -> Broadcast: Beacon Request
    For 'Test procedure' item 2:
      2.1. ZC -> Broadcast: Beacon
      2.2. ZR -> ZC: Association
      2.3. ZR -> ZC: BDB commissioning
      2.4. ZR -> ZC: Match Descriptor Request
      2.5. ZC -> ZR: Match Descriptor Response
      2.6. ZC -> Broadcast: Image Notify
      2.7. ZR -> ZC: Query Next Image Request
      2.8. ZC -> ZR: Query Next Image Response
      2.9. ZR -> ZC: Image Block Request. ZC -> ZR: Image Block Response (x3)
      2.10. ZR -> ZC: Upgrade End Request
      2.11. ZC -> ZR: Upgrade End Response
