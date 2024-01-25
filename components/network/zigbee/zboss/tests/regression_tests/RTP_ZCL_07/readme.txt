Bug 12560 - ZCL callback with more than one attribute (color control cluster) 
RTP_ZCL_07 - Color Control cluster callbacks should be called with appropriate attr_id after command with more than one attribute

Objective:

	To confirm that the device call Color Control cluster callbacks with appropriate attr_id after command with more than one attribute.

Devices:

	1. TH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR and TH ZC complete finding and binding procedure
	4. Wait for DUT ZR receives ZCL Color Control "Move to Hue and Saturation" command
	5. Wait for DUT ZR print to trace arguments of Color Control cluster callbacks and respond with Default Response 

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

	3. TH ZC performs finding and binding (TH ZC as initiator, DUT ZR as target):
       TH ZC binds to the Color Control cluster of DUT ZR.

	4. DUT ZR receives ZCL Color Control "Move to Hue and Saturation" command

	5. DUT ZR prints to trace arguments of Color Control cluster callbacks. 
	The trace should contain the following strings:

		- test_check_value_color_control_srv: attr_id=0, value=X
		- test_attr_hook_color_control_srv: attr_id=0, new_value=X 
			(it is possible to check that new_value is equal to value from previous line)

		- test_check_value_color_control_srv: attr_id=1, value=X
		- test_attr_hook_color_control_srv: attr_id=1, new_value=X
			(it is possible to check that new_value is equal to value from previous line)

	6. DUT ZR responds with the Default Response with the Success status (0x00)