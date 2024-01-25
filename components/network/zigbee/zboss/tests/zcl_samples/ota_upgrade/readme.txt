    ZBOSS Zigbee software protocol stack

    Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
    http://www.dsr-zboss.com
    http://www.dsr-corporation.com
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


ZBOSS OTA Upgrade Sample application
================================

This application is a Zigbee Router device which contains all clusters
from OTA Upgrade set (OTA Upgrade).
Every supported attribute is declared for the cluster (both for server
and client).
The main test loop sends all of the supported client request commands.
Default operational channel is set to 22.

OTA Upgrade Sample application includes following ZCL clusters:

  - OTA Upgrade (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- ota_upgrade_zr.c - *OTA Upgrade Sample application*
- ota_upgrade_zr.h - *OTA Upgrade Sample application header file*

Application behavior
---------------------

After starting up, device will start sending every 50 ms, one by one, all of
the supported request commands of OTA Upgrade client cluster.