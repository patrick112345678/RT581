Revision 50722 (R21) - Child devices counting fix 
RTP_SEC_05 - device stores correct number of child devices

Objective:

	To confirm that the device will store correct number of child devices after it does not receive respond for Transport Key during commissioning

Devices:

	1. DUT - ZC
	2. CTH - ZR
	3. CTH - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
	2. Power on CTH ZR
	3. Wait for DUT ZC and CTH ZR perform association attempt and DUT ZC send Transport Key
	4. Power off CTH ZR (it should not send any requests after receiving Transport Key)
	5. Power on CTH ZED
	6. Wait for DUT ZC and CTH ZED perform association attempt and DUT ZC send Transport Key
	7. Power off CTH ZED (it should not send any requests after receiving Transport Key)
	8. Wait for DUT ZC print to trace its children count

Expected outcome:

	1. DUT ZC creates a network

	2. CTH ZR starts bdb_top_level_commissioning and tries to join to the network established by DUT ZC

	3. DUT ZC sends Transport Key during commissioning

	4. DUT ZC does not receive any requests from CTH ZR

	5. CTH ZED starts bdb_top_level_commissioning and tries to join to the network established by DUT ZC

	6. DUT ZC sends Transport Key during commissioning

	7. DUT ZC does not receive any requests from CTH ZED

	8. DUT ZC prints to trace its children count. The trace should contain the following string:
		- The device has 0 child routers, 0 child eds
