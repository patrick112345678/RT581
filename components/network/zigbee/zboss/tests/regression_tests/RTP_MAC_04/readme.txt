Bug 12781 - How to get/set extpanid 
RTP_MAC_04 - ext_pan_id API works correctly (zb_set_extended_pan_id in ZC)

Objective:

	To confirm that the function zb_set_extended_pan_id allows to set extended PAN ID in the coordinator

Devices:

	1. DUT - ZC
	2. TH - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC and TH ZR
	2. Wait for DUT ZC and TH ZR association

Expected outcome:

	1. DUT ZC prints to trace string "test_get_extended_pan_id_on_start: bb.bb.bb.bb.bb.bb.bb.bb"

	2. DUT ZC creates a network

	3. DUT ZC prints to trace string "test_get_extended_pan_id_on_steering: bb.bb.bb.bb.bb.bb.bb.bb"

	4. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC
	   During association DUT ZC sends Beacon to TH ZR. Extended PAN ID in the Beacon must be equal to "bb.bb.bb.bb.bb.bb.bb.bb"