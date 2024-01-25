Bug 11656 - Unable to handle group commands after 09/04/2018 ZBOSS update 
RTP_ZCL_12 - zcl_device_cb is called on group-cast frame reception

Objective:

	To confirm that the device calls zcl_device_cb after group-cast frame reception.

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR and TH ZC complete finding and binding procedure
	4. Wait for DUT ZR receive Add Group Request and respond with Add Group Response
	5. Wait for DUT ZR receive and handle group-cast "Move to Hue and Saturation" command from ZCL Color Control cluster
	6. Wait for DUT ZR receive and handle group-cast OnOff Toggle command from OnOff cluster

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. TH ZC performs finding and binding (TH ZC as initiator, DUT ZR as target)

	4.1. DUT ZR receives Add Group Request:
		Group ID: 0xaaaa

	4.2. DUT ZR responds with Add Group Response:
		Group Status: Success (0x00)
		Group ID: 0xaaaa

	5.1. DUT ZR receives group-cast "Move to Hue and Saturation" command (group = 0xaaaa)
	5.2. DUT ZR prints to trace the following strings:
		- zcl_device_cb for set attr is called: clusterid=0x300, attrid=0x0
		- zcl_device_cb for set attr is called: clusterid=0x300, attrid=0x1

	6.1. DUT ZR receives group-cast OnOff Toggle command (group = 0xaaaa)
	6.2. DUT ZR prints to trace the following strings:
		- zcl_device_cb for set attr is called: clusterid=0x6, attrid=0x0