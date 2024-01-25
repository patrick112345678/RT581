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


ZBOSS Lighting Sample application
================================

This application is a Zigbee Router device which contains all clusters
from Lighting set (currently, only Color Control cluster is supported).
Every supported attribute is declared for the cluster (both for server
and client).
The main test loop sends all of the supported client request commands.
Default operational channel is set to 22.

Lighting Sample application includes following ZCL clusters:

  - Color Control (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- lighting_zr.c - *Lighting Sample application*
- lighting_zr.h - *Lighting Sample application header file*

Application behavior
---------------------

After starting up, device will start sending every 50 ms, one by one, all of
the supported client request commands of Color Control cluster.