Bug 11044 - Unable to join network after board reset 
RTP_SEC_04 - The coordinator with configurable memory restores network state after board reset

Objective:

	To confirm that the coordinator with configurable memory can restore network state after board reset

Devices:

	1. DUT - ZC
	2. TH - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. All devices use configurable memory
        3. DUT ZC does not erase NVRAM at start

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZED
        3. Wait for TH ZED join to the network created by DUT ZC and perform keys exchange
        4. Power off DUT ZC
        5. Power on DUT ZC
        6. Wait for DUT ZC receive Buffer Test Request and respond with Buffer Test Response 

Expected outcome:

	1. DUT ZC creates a network

        2. TH ZED joins to the network created by DUT ZC and perform key exchange

        3. DUT ZC powers off and on

        4. DUT ZC receives Buffer Test Request and responds with Buffer Test Response