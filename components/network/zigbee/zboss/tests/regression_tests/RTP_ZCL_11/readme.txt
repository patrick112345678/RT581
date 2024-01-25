Bug 11709 - ZC not re-transmitting Read Attribute Response
RTP_ZCL_11 - forwarding ZCL packets

Objective:

	To confirm that ZC successfully forwards packets from ZR.

Devices:

	1. DUT - ZC
	2. CTH - ZR
	2. CTH - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on CTH ZED
        3. Power on CTH ZR
        4. Wait for read attribute ZCL command from CTH ZED to CTH ZR
        5. Wait for read attribute response ZCL command from CTH ZR to CTH ZED
        6. Wait for read attribute ZCL command from CTH ZR to CTH ZED
        5. Wait for read attribute response ZCL command from CTH ZED to CTH ZR

Expected outcome:

	1. DUT ZC creates a network

        2. DUT ZC performing f&b as a target

	3. (Test procedure 5) DUT ZC forwards Read Attribute Response to CTH ZED

	4. (Test procedure 6) DUT ZC forwards Read Attribute to CTH ZED
