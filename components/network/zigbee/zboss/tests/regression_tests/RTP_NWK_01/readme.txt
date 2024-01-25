Revision 50623 (R21) - Beacon payload update fix 
RTP_NWK_01 - device sets Association Permit to false if it already has max children

Objective:

	To confirm that the device will set Association Permit to false if it already has max children

Devices:

	1. DUT - ZC
	2. TH - ZR1
	3. TH - ZR2

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. DUT ZC can have no more than one child

Test procedure:

	1. Power on DUT ZC and TH ZR1
	2. Wait for DUT ZC and TH ZR1 association
	3. Power on TH ZR2
	4. Wait for DUT ZC receive Beacon Request from TH ZR2
	5. Wait for DUT ZC respond with Beacon

Expected outcome:

	1. DUT ZC creates a network

	2. TH ZR1 starts bdb_top_level_commissioning and gets on the network established by DUT ZC

	3. DUT ZC receives Beacon Request from TH ZR2

	4. DUT ZC responds with Beacon with Association Permit equal to false