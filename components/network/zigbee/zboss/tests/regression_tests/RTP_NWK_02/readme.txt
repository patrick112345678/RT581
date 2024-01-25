Revision 50622 (R21) - Routing fix 
RTP_NWK_02 - device uses established route for request with NWK FC Discover Route equal to 0

Objective:

	To confirm that the device will use established route for sending request with NWK FC Discover Route equal to 0 if such route is existing

Devices:

	1. DUT - ZR
	2. TH - ZC
	3. TH - ZR
	4. TH - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. Each device adds all devices except neighbors in blacklist
	3. All devices has fixed addresses:
		TH_ZC: 0x0000
		TH_ZR: 0x0001
		DUT_ZR: 0x0002
		TH_ZED: 0x0003

Test procedure:

	1. Power on TH ZC, TH ZR, DUT ZR, TH_ZED
	2. Wait for TH ZC and TH ZR association
	3. Wait for TH ZR and DUT ZR association
	4. Wait for DUT ZR and TH ZED association
	5. Wait for DUT ZR receive Buffer Test Request intended for TH ZC from TH ZED and forward it to TH ZR
	6. Wait for DUT ZR receive Buffer Test Response intended for TH ZED from TH ZR and forward it to TH ZED

Expected outcome:

	1. TH ZC creates a network

	2. TH ZR starts bdb_top_level_commissioning and joins to the network established by TH ZC

	3. DUT ZR starts bdb_top_level_commissioning and joins to the TH ZR as a child

	4. TH ZED starts bdb_top_level_commissioning and joins to the DUT ZR as a child

		At this moment the network should has the following structure: TH ZC -> TH ZR -> DUT ZR -> TH ZED

	5.1. DUT ZR receives Buffer Test Request intended for TH ZC from TH ZED. 
		 Discover Route field in NWK Frame Control should be equal to 0
	5.2. DUT ZR forwards Buffer Test Request to TH ZR

	6.1. DUT ZR receives Buffer Test Response intended for TH ZED from TH ZR
	6.2. DUT ZR forwards Buffer Test Response to TH ZED