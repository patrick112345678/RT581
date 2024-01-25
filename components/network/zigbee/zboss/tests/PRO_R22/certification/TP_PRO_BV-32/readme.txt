TP/PRO/BV-32 Network Commissioning – EPID storage (Power failure)
Objective: To confirm the Extended PANid of a device is correctly stored in non-volatile storage.

gZC
DUT ZR1

Initial condition:
1. Reset all nodes
2. Set gZC under target stack profile, gZC as coordinator starts a PAN = 0x1AAA network; is the Trust Centre for the PAN.
3. DUT ZR1 is configured with:
   NwkExtendedPANID = 0x0000 0000 0000 0001
   apsDesignatedCoordinator = False
   apsUseExtendedPANID = 0x0000 0000 0000 0001
   apsUseInsecureJoin = TRUE

Test Procedure:
1. Restart DUT ZR1.
2. Unicast Buffer Test Request (0x001c) from gZC to DUT ZR1.

Pass verdict:
1) DUT ZR1 shall restart and resume normal PAN operation.
2) gZC shall unicast Buffer Test Request (0x001c) to DUT ZR1
3) DUT ZR1 shall unicast Buffer Test Response (0x0054) to gZC

Fail verdict:
1) DUT ZR1 does not restart and resume normal PAN operation.
2) gZC does not unicast Buffer Test Request (0x001c) to DUT ZR1
3) DUT ZR1 does not unicast Buffer Test Response (0x0054) to gZC

Comments:
Test procedure: has 2 devices - ZC and ZR
ZC does Formation procedure, including Energy and Active scan.
ZR does Discovery then Join to the network at ZC.
ZR resets and sends Test Buffer Request (w/o any joining procedure)


Comments:
ZC sends test buffer request to the router after 40sec. timeout;
