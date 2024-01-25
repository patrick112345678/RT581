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


ZBOSS Security and Safety Sample application
================================

This application is a Zigbee Router device which contains all supported
clusters from Security and Safety set. Every currently supported attribute is declared for
each cluster (both server and client). The main test loop sends all of the
supported client request commands of each cluster.
Default operational channel is set to 22.

Security and Safety Sample application includes following ZCL clusters:

 - IAS Zone (s/c)
 - IAS ACE (s/c)
 - IAS WD (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- security_safety_zr.c - *Security and Safety Sample application*
- security_safety_zr.h - *Security and Safety Sample application header file*

Application behavior
---------------------

After starting up, device will start sending every 50 ms, one by one, all of
the supported request commands of all the client clusters declared in this
application.

The application also implements status response logic for Squwak and Star Warning
commands of IAS WS cluster server and Enroll Response command of IAS Zone cluster
server, which are not handled by the ZBOSS stack.