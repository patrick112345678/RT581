Uni-directional commissioning demo at CC2538.
HW: CC2538 + SmartRF06 devkit, Legrand switch.

Supports Legrans switch (uses uni-directional Commissioning frame)
and DSR ZGPD (uses auto-commissioning grame).

For Legrand supports ON/OFF commands and Scene commands.
ON - hold 2 upper butons + power.
OFF  - hold 2 lower butons + power.
Scene 0-4 one button + power.

At CC2538 uses 2 buttons:
Left - start commissioning
Right - stop commissioning.

Commissioning timeout - infinite (leave commissioning mode after one
device commissionid).

LED indications:
- LED4 blinking - in commissioning mode.
- LED1 - ON/OFF switch state
- LED2, LED3 - binary representation of received Scene # in received
Scene command.

Scene0 - Scene4 are supported. Scene5 and above are ignored.

