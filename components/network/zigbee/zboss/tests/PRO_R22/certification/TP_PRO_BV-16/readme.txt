TP/PRO/BV-16 Address Conflict Resolution by latest joining device
Verify that a router joining a PAN, correctly changes its short address if it detects
another device on the PAN already has the same short address.

gZC
gZR1
DUT ZR2

Test procedure:
1. gZC goes offline;
2. gZR1 shall change its address to that of DUT ZR2 and send out an Device_announce with the new address;

Pass verdict:
1. On receipt of a Device announce command from gZR1, DUT ZR2 shall change its short address;
2. DUT ZR2 shall broadcast a Device Announce with new short address;
3. DUT ZR2 shall send out a Network status command for the address conflict;

Fail verdict:
1. DUT ZR2 does not join PAN at gZC (gZR1 and gZC do not ignore the Device Announce);
2. On receipt of a Network Link Status command from gZR1, DUT ZR2 does not change its short address;
3. DUT ZR2 does not broadcast a Device Announce with new short address;


Test procedure at HW:
- start zc
- start zr1, wait for join
- start zr2, wait for join
- interrupt zc (it is enough - not need to switch hw off)
- wait for device annce from zr1 and address change