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



ZBOSS Simple Combo set of applications
========================================

ZB PRO functionality with HA profile and Green
Power Basic proxy-basic commissioning Demo.
Supported ZGPS: GreenPeak ZGPS supprting bi-directional commissioning
(door/window sensor, leak, humidity etc), Legrand switch.

Tested with following ZGPDs:
- Legrand switch (uni-directional commissioning frames, on/off, scenes).
- DSR ZGPD (uses auto-commissioning frames).
- GreenPeak Door Window Sensor
- GreenPeak Temperature Sensor
- GreenPeak Water Leakage sensor


The application set structure
------------------------------

 - Makefile 
 - zc_combo.c - *Simple Combo coordinator application*
 - zr_proxy.c - *Proxy router application*
 - simple_combo_match.h
 - simple_combo_zcl.h
 - readme.txt

Direct communication with ZGPD is blocked at Combo to be able to
demonstrate multi-hop commissioning by zb_zgp_set_skip_gpfd(ZB_TRUE).
Change to zb_zgp_set_skip_gpfd(ZB_FALSE) to work with ZGPD directly.

If buttons supported, use 2 buttons:
Left - start commissioning
Right - stop commissioning.

If no buttons supported, commissioning start 12 seconds after
Formation complete and allows only 1 ZGPD device to commission. Then
GPCB leaves commissioning mode and starts processing of data events
from ZGPD.

Commissioning timeout is infinite (leave commissioning mode after one
device commissioned).

LED indications, when supported by HW:
- LED4 blinking - in commissioning mode.
- LED1 - ON/OFF switch state
- LED2, LED3 - binary representation of received Scene # in received
Scene command.

Scene0 - Scene4 are supported. Scene5 and above are ignored.
