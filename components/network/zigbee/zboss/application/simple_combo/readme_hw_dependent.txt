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


Uni-directional commissioning demo.
----------------------------------------------------------------------------
Supports Legrans switch (uses auto-commissioning frame)
and DSR ZGPD (uses uni-directional Commissioning frame).

For Legrand supports ON/OFF commands and Scene commands.
ON - hold 2 upper butons + power.
OFF  - hold 2 lower butons + power.
Scene 0-4 one button + power.

If buttons are supported, zc_combo app will use 2 buttons:
Button 0 (Left) - start commissioning
Button 1 (Right) - stop commissioning
Else zc_combo will start commissioning automatically in 12 sec after startup.

Commissioning timeout - infinite (leave commissioning mode after one
device commissionid).

If LED indication is supported:
- LED4 blinking - in commissioning mode.
- LED1 - ON/OFF switch state
- LED2, LED3 - binary representation of received Scene # in received
Scene command.

Scene0 - Scene4 are supported. Scene5 and above are ignored.

Legrand switch operation:
- remove the cover; Switch top is marked by '0', bottom by 'I'.
- commissioning initiation: hold left down button and click piezo
button for >10 seconds. Repeat to commission at the next channel.
- commissioning done: hold left up button and click piezo button.
- ON: hold 2 upper butons + piezo.
- OFF: hold 2 lower butons + piezo.
- Scene 0-4:  one button + piezo.

GreenPeak GPDs operation:
- press a button for commissioning start.
- to decommission press and hold a button.


Test commissioning with GreenPeak sensor:
- Launch Device Manager GUI
- Switch on ZC, check USB serial name
- Switch on ZR, check USB serial name
- Switch off ZC and ZR
- Start win_com_dump collecting traces from both devices
- Start sniffer
- Switch on ZC
- Switch on ZR
- Observe traffic using sniffer, wait for rejoin complete
- Press Left button at ZC. Ensure it enabled commissioning using
sniffer and LED4
- Press Commissioning button at GPD

----------------------------------------------------------------------------
Use cases:
1. Direct commissioning to zc_combo.
Configure next flags in zc_combo.c:
  ZG->nwk.skip_gpdf = 0;
Rebuild the application. Start device and turn on commissioning (or wait 15-20 sec), then turn on
commissioning on ZGPD side.
2. Commissioning through zr_proxy.
Configure next flags in zc_combo.c:
  ZG->nwk.skip_gpdf = 1;
Rebuild the application, flash zc_combo and zr_proxy on 2 different devices. Start zc_combo and then
start zr_proxy before commissioning start on zc_combo. After the start of commissioning, turn it on
on ZGPD.
