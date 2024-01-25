Bug 12785 - Impossible to toggle the bulb if the bulb has only mandatory attributes 
RTP_ZCL_06 - it is possible to toggle the on-off cluster if it has only mandatory attributes

Objective:

	To confirm that device can correctly handle On Off Toggle command if On Off cluster has only mandatory attributes

Devices:

	1. TH - ZC
	2. DUT  - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. DUT ZED has On Off cluster with only mandatory attribute OnOff
        3. OnOff attribute initial value is On (0x01)

Test procedure:

	1. Power on TH ZC
        2. Power on DUT ZED
        3. Wait for TH ZC bind with DUT ZED
        4. Wait for DUT ZED receive On Off Toggle command and responds with the Default Response
        5. Wait for DUT ZED receive Read Attributes Request for OnOff attribute and responds with the Read Attributes Response
        6. Wait for DUT ZED receive On Off Toggle command and responds with the Default Response
        7. Wait for DUT ZED receive Read Attributes Request for OnOff attribute and responds with the Read Attributes Response

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

        3. TH ZC starts finding and binding procedure as initiator and binds with DUT ZED

        4. DUT ZED receive On Off Toggle command from TH ZC

        5. DUT ZED toggles OnOff attribute to Off and responds with Default Response:
               Status: Success (0x00)

        6. DUT ZED receives Read Attributes Request from TH ZC:
               Command: Read Attributes (0x00)
               Attribute: OnOff (0x0000)

        7. DUT ZED responds with the Read Attributes Response:
               Attribute: OnOff (0x0000)
               Status: Success (0x00)
               On/off Control: Off (0x00)

        8. DUT ZED receive On Off Toggle command from TH ZC

        9. DUT ZED toggles OnOff attribute to On and responds with Default Response:
               Status: Success (0x00)

        10. DUT ZED receives Read Attributes Request from TH ZC:
               Command: Read Attributes (0x00)
               Attribute: OnOff (0x0000)

        11. DUT ZED responds with the Read Attributes Response:
               Attribute: OnOff (0x0000)
               Status: Success (0x00)
               On/off Control: On (0x01)