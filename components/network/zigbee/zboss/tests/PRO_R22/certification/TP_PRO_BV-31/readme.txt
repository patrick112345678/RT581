TP/PRO/BV-31 Network Commissioning – EPID rejoin (Rejoin procedure)
Objective: To confirm the network start-up with respect to Extended PANId.

gZC
DUT ZR1
DUT ZR2

Initial Conditions:
1. Set gZC under target stack profile, gZC as coordinator
starts a PAN = 0x1AAA network; is the Trust Centre for the PAN.
2. DUT ZR1 is configured with:
   NwkExtendedPANID = 0x0000 0000 0000 0000
   apsDesignatedCoordinator = False
   apsUseExtendedPANID = 0x0000 0000 0000 0001
   apsUseInsecureJoin = TRUE;
3. DUT ZR2 is configured with:
   NwkExtendedPANID = 0x0000 0000 0000 0000
   apsDesignatedCoordinator = False
   apsUseExtendedPANID = 0x0000 0000 0000 1111
   apsUseInsecureJoin = TRUE;

Test procedure:
1. While gZC has JOIN = True, start up DUT ZR1;
2. While gZC has JOIN = True, start up DUT ZR2;
3. gZC shall unicast a Buffer Test Request (0x001c) to DUT ZR1;

Pass verdict:
1) DUT ZR1 shall transmit a Beacon Request MAC command.
2) gZC shall transmit a Beacon
3) DUT ZR1 shall unicast a Rejoin Request command to gZC
4) gZC shall unicast a Rejoin Response command to DUT ZR1
5) DUT ZR1 broadcasts and EndDevAnnce
6) DUT ZR2 shall transmit a Beacon Request MAC command.
7) gZC shall transmit a Beacon
8) DUT ZR2 shall not join PAN.
9) gZC shall unicast a Buffer Test Request (0x001c) command to DUT ZR1
10) DUT ZR1 shall unicast a Buffer Test Response (0x0054) command to gZC

Fail verdict:
1) DUT ZR1 does not transmit a Beacon Request MAC command.
2) gZC does not transmit a Beacon
3) DUT ZR1 does not unicast a Rejoin Request command to gZC
4) gZC does not unicast a Rejoin Response command to DUT ZR1
5) DUT ZR1 does not broadcast an EndDevAnnce
6) DUT ZR2 does not transmit a Beacon Request MAC command.
7) gZC does not transmit a Beacon
8) DUT ZR2 does join PAN at gZC (in the absence of EPID = 0x0000 0000 0000 1111)
9)	gZC does not unicast a Buffer Test Request (0x001c) command to DUT ZR1
10)	DUT ZR1 does not unicast a Buffer Test Response (0x0054) command to gZC

Comments:
1) gZC starts with EPID = 0x0000 0000 0000 0001
2) DUTZR1 starts with EPID = 0x0000 0000 0000 0001 and joins gZC via rejoin procedure
3) DUTZR2 starts with EPID = 0x0000 0000 0000 1111 and fails to join gZC
4) gZC sends buffer test request
5) DUTZR1 answers with buffer test response


Features:
NLF71 Initiate Rejoin and join by association

