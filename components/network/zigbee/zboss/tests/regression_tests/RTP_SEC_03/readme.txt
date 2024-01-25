Bug 11044 - Unable to join network after board reset 
RTP_SEC_03 - Device with configurable memory joins network after board reset

Objective:

	To confirm that end device with configurable memory can join network after board reset

Devices:

	1. TH  - ZC
	2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. All devices use configurable memory
        3. DUT ZED does not erase NVRAM at start

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZED
        3. Wait for DUT ZED join to the network created by TH ZC and perform keys exchange
        4. Power off DUT ZED
        5. Power on DUT ZED
        6. Wait for DUT ZED rejoin to the network created by TH ZC

Expected outcome:

	1. TH ZC creates a network

        2. DUT ZED joins to the network created by TH ZC and perform key exchange

        3. DUT ZED powers off and on

        4. DUT ZED rejoins to the network created by TH ZC