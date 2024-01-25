Bug 13182 - Finding & Binding between ZHA (0x0104) and ZLL (0xC05E) profiles
RTP_ZCL_05 - Device with ZLL profile binds to device with ZHA profile

Objective:

	To confirm that the device with ZLL profile can successfully be bound to device with HA profile.

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. TH ZC has one endpoint with ZLL profile
	3. DUT ZR has one endpoint with HA profile
	4. All devices are built with define ZB_ENABLE_ZLL

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZR
	3. Wait for DUT ZR receive Identify Query and send Identify Query Response
	4. Wait for DUT ZR receive Simple Descriptor Request and send Simple Descriptor Response
	5. Wait for DUT ZR send Identify Query

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. TH ZC starts finding and binding as initiator, DUT ZR as target

	4. DUT ZR receives Identify Query and send Identify Query Response

	5. DUT ZR receive Simple Descriptor Request and sends Simple Descriptor Response

	6. DUT ZR and TH ZC finishes finding and binding

	7. DUT ZR starts finding and binding as initiator, TH ZC as target

	8. DUT ZR sends Identify Query and does not receive Identify Response
