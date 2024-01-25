Bug 12781 - How to get/set extpanid 
RTP_MAC_03 - ext_pan_id API works correctly (zb_get_extended_pan_id in ZR)

Objective:

	To confirm that the functions zb_get_extended_pan_id allows to get actual extended PAN ID for device after association

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC and DUT ZR
	2. Wait for TH ZC and DUT ZR association
	3. Wait for DUT ZR print to trace actual PAN ID

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC
		
	3. DUT ZR prints to trace string "test_get_extended_pan_id: bb.bb.bb.bb.bb.bb.bb.bb"