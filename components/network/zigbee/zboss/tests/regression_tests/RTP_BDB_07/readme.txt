Bug 14310 - Router crashes after 900s when there is no coordinator - light_bulb
RTP_BDB_07 - router will work if the coordinator turns off

Objective:

	To confirm that the router (child of the coordinator) that has child end device will work after coordinator will turn off.

Devices:

	1. DUT - ZR
	2. TH  - ZC
	3. TH  - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. All devices do not erase NVRAM at start

Test procedure:

	1. Power on TH ZC
	2. Power on TH ZED
	3. Power on DUT ZR
	4. Wait for TH ZED and DUT ZR associate with TH ZC
	5. Power off TH ZC
	6. Wait for TH ZED rejoin to the DUT ZR (around 60 seconds)
	7. Power off DUT ZR and TH ZED
	8. Power on DUT ZR and TH ZED
	9. Wait for 900 seconds
	10. Power on TH ZC
	11. Wait for DUT ZR receive Link Quality Request from TH ZC and sends Link Quality Response

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR and TH ZED successfully joined and authorized with TH ZC

	3. TH ZED rejoins from TH ZC to DUT ZR after delay (around 60 seconds)

	4. DUT ZR starts to receive data requests from TH ZED

	5. DUT ZR continues to work and receive data requests from TH ZED after power off and on during all the time

	6. TH ZC returns to the network after power on

	7. DUT ZR receives Link Quality Request from TH ZC and sends Link Quality Response

	8. DUT ZR continues to work and receive data requests from TH ZED