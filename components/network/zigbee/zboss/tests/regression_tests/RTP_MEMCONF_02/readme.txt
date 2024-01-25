Bug 11341 - New error with 07/03/2018 libraries 
RTP_MEMCONF_02 - device with memconfig correctly clear contexts after leave request and does not crash

Objective:

	To confirm that device with will memconfig correctly clear contexts after leave request and does not crash

Devices:

	1. TH  - ZC
	2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. DUT ZED does not erase NVRAM at start

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZED
        3. Wait for DUT ZED receive Leave Request and responds with Leave Response
        4. Power off DUT ZED
        5. Power on DUT ZED
        6. Wait for DUT ZED join to the network created by TH ZC

Expected outcome:

	1. TH ZC creates a network

        2. DUT ZED joins to the network created by TH ZC

        3. DUT ZED receives Leave Request from TH ZC and responds with Leave Response with Success status

        4. DUT ZED powers off and on

        5. DUT ZED joins to the network created by TH ZC