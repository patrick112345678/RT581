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


ZBOSS Closures Sample application
================================

This application is a Zigbee Router device which contains all supported
clusters from Closures set (both server and client). Every supported
attribute is declared for each cluster (both server and client).
The main test loop sends all of the supported client request commands
of each cluster.
Default operational channel is set to 22.

Closures Sample application includes following ZCL clusters:

  - Shade Configuration (s/c)
  - Door Lock (s/c)
  - Window Covering (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- closures_zr.c - *Closures Sample application*
- closures_zr.h - *Closures Sample application header file*

Application behavior
---------------------

After starting up, device will start sending every 50 ms, one by one, all of
the supported request commands of all the client clusters declared in this
application.

This application also implements Door Lock/Unlock state update logic for
Door Lock and Door Unlock commands of the Door Lock cluster server, which
is not handled by the ZBOSS stack.
