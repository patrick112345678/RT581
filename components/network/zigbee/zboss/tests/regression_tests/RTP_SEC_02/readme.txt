Bug 11341 - New error with 07/03/2018 libraries 
RTP_SEC_02 - device correctly deals with races between reaction on LEAVE and TCLK update wait timeout

Objective:

	To confirm that device will not crash on Leave command after 3 Request Key attempts

Devices:

	1. CTH  - ZC
	2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. DUT ZED does not erase NVRAM at start

Test procedure:

	1. Power on CTH ZC
        2. Power on DUT ZED
        3. Wait for DUT ZED performs 3 Request Key attempts (CTH ZC should not respond with Transport Key)
        4. Wait for DUT ZED receive Leave command
        5. Power off DUT ZED
        6. Power on DUT ZED
        7. Wait for DUT ZED join to the network created by CTH ZC

Expected outcome:

	1. CTH ZC creates a network

        2. DUT ZED starts to associate with CTH ZC

        3. DUT ZED sends Request Key 3 times and does not receive Transport Key after these requests

        4. DUT ZED receives Leave command

        5. DUT ZED powers off and on

        6. DUT ZED joins to the network created by CTH ZC