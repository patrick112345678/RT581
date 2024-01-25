Bug 10658 - Controlling ZigBee groups via ZBOSS API 
RTP_ZCL_14 - Device use group-cast request to send ZCL command to a group.

Objective:

	To confirm that the device will use group-cast request to send manufacturer specific ZCL command to a group (not a multi-cast request).

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
	5. Wait for DUT ZR receive and handle manufacturer specific group-cast command from ZCL Scenes cluster

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. TH ZC performs finding and binding (TH ZC as initiator, DUT ZR as target)

	4.1. DUT ZR receives Add Group Request:
		Group ID: 0xaaaa

	4.2. DUT ZR responds with Add Group Response:
		Group Status: Success (0x00)
		Group ID: 0xaaaa

	5.1. DUT ZR receives manufacturer specific group-cast command with custom payload:
		Destination: Broadcast
		Delivery Mode: Group (0x3)
		Group: 0xaaaa
		Manufacturer Code: Unknown (0x117c)
		Command: 0x07 (Manufacturer specific)
		Data: 0xaa 0x01 0x0d 0x00 (custom payload)

	5.2. DUT ZR prints to trace the following strings:
		- Manufacturer specific command is received and processed