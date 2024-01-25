Bug 12092 - Device dies after several Write Attributes Attempts
RTP_ZCL_09

Objective:

    To confirm that the write attribute request does not cause memory crash.

Devices:

	1. TH  - ZC
	2. DUT - ZR
    	3. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZR
        3. Power on TH ZR
        4. TH ZR sends 300 Write Attribute ZCL commands to DUT ZR
           Command: Write Attributes (0x02)
               Attribute: Location Description (0x0010)
               Data Type: Character String (0x42)
               String: Conference Room

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR and TH ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

        3. (Test procedure 4) DUT ZR receives 300 Write Attribute Response ZCL commands with success status in response to Write Attribute ZCL Commands from TH ZR
           Command: Write Attributes Response (0x04)
               Status: Success (0x00)
