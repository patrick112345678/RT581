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


ZBOSS Measurement and Sensing Sample application
================================

This application is a Zigbee Router device which contains all supported
clusters from Measurement and Sensing set. Every currently supported attribute is declared for
each cluster (both server and client). The main test loop sends all of the
supported client request commands of each cluster.
Default operational channel is set to 22.

Measurement and Sensing Sample application includes following ZCL clusters:

 - Illuminance Measurement (s/c)
 - Tempereture Measurement (s/c)
 - Water Content Measurement (s/c)
 - Occupancy Sensing (s/c)
 - Electrical Measurement (s/c)

The application folder structure
--------------------------------

- open-pcap.sh - *The script for openning logs*
- readme.md - *This file*
- measurement_sensing_zr.c - *Measurement and Sensing Sample application*
- measurement_sensing_zr.h - *Measurement and Sensing Sample application header file*

Application behavior
---------------------

None of the clusters clients supports sending commands, so the device will stay iddle
after start.