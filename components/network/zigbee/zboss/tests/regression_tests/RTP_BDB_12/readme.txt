Bug 11190 - Calling bdb_start_top_level_commissioning() during active association crashes stack 
RTP_BDB_12 - Calling bdb_start_top_level_commissioning() during active association

Objective:

	To confirm that bdb_start_top_level_commissioning() returns error if it is called during active association.

Devices:

	1. TH  - ZC
	2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. TH ZC performs install codes checking

Test procedure:

	1. Power on TH ZC
    2. Power on DUT ZED
	3.1. Wait for DUT ZED send Association Request
	3.2. Wait for DUT ZED receive Association Response

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZED tries to associate with TH ZC

	3.1. DUT ZED sends Association Request
	3.2. DUT ZED receives Association Response
	3.3. DUT ZED prints to trace the following string:
		- can not start bdb top level commissioning during active association