TP/PRO/BV-15 Address Conflict Resolution in multihop network
Verify that in a multihop network, many nodes detect and resolve the address conflict.

         DUT ZC
   |              |
DUT ZR1        DUT ZR3
   |              |
  gZR2           gZR4

* Need to use Test Profile 3;
**Assign the short address of gZR4 as same as that of gZR2 by some means;

Test procedure:
1. gZR4 shall join the PAN at DUT ZR3 gZR4 the same short address as gZR2 (using Test Profile 3);

Pass verdict:
1. gZR4 shall broadcast a Device Announce with short address same as gZR2;
2. DUT ZR3, DUT ZC or DUT ZR1 shall broadcast a Network Status command with status 0x0d (address conflict) 
and destination address the same as short address of gZR2/gZR4;
3. gZR4 shall change its short address;
4. gZR4 shall broadcast a Device Announce command with new short address;
5. gZR2 shall change its short address
6. gZR2 shall broadcast a Device Announce command with new short address

Fail verdict:
1. gZR4 does not broadcast a Device Announce with short address same as gZR2;
2. Neither DUT ZR3 nor DUT ZC broadcast a Network Status command with
status 0x0d (address conflict) and destination address the same as short
address of gZR2/gZR4.
3. gZR4 does not change its short address;
4. gZR4 does not broadcast a Device Announce command with new short address;
5. gZR2 does not change its short address;
6. gZR2 does not broadcast a Device Announce command with new short address;



