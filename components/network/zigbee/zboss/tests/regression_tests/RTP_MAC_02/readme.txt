Bug 14250 - Weird data-request behaviour
RTP_MAC_02 - parent device does not send non-existing data after data request if it was turned off and then turned on

Objective:

	To confirm that the parent device will not send non-existing data after data request from child device if parent was turned off adn then turned on.

Devices:

	1. TH  - ZED
	2. DUT - ZC

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
	2. Power on TH ZED
	3. Wait for DUT ZC and TH ZED associate and TH ZED start to send data requests
	4. Turn off DUT ZC for 0.5 - 6 seconds
	5. Turn on DUT ZC before TH ZED start sending beacon request
    6. Wait for DUT ZC receive data request

Expected outcome:

	1. DUT ZC creates a network

	2. TH ZED starts bdb_top_level_commissioning and gets on the network established by DUT ZC

	3. DUT ZC receives at least one data request and responds with ack (with Frame Pending = false)

	4. DUT ZC turns off for 0.5 - 6 seconds and turns on before TH ZED start sending beacon request

	5. DUT ZC receives at least one data request and responds with ack (with Frame Pending = false)