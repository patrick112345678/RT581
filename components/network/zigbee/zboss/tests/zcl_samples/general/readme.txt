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


ZBOSS General Sample application
================================

This application is a Zigbee Router device which contains all supported
clusters from General set. Every currently supported attribute is declared
for each cluster (both server and client). The main test loop sends all of
the supported client request commands of each cluster.
Default operational channel is set to 22.

General Sample application includes following ZCL clusters:

  - On/Off (s/c)
  - Basic (s/c)
  - Identify (s/c)
  - Groups (s/c)
  - Scenes (s/c)
  - On/Off Switch Config (s/c)
  - Level Control (s/c)
  - Power Config - Battery (s/c)
  - Power Config - Mains (s/c)
  - Alarms (s/c)
  - Time (s/c)
  - Binary Input (s/c)
  - Diagnostics (s/c)
  - Poll Control (s/c)
  - Meter Identification (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- general_zr.c - *General Sample application*
- general_zr.h - *General Sample application header file*

Application behavior
---------------------

After starting up, device will start sending every 50 ms, one by one, all of
the supported request commands of all the client clusters declared in this
application.

The application also implements status response logic for Reset Alarm and
Reset All Alarms commands of the Alarms cluster server, which is not handled
by the ZBOSS stack.
