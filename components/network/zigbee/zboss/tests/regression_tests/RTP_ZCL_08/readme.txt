Bug 12340 - Issues inside ZCL reporting feature
RTP_ZCL_08 - min and max interval values of zcl reporting

Objective:

	To confirm that the minimum and maximum interval values of the configure reporting ZCL command are handled properly.

Devices:

	1. CTH - ZC
	2. DUT - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on CTH ZC
        2. Power on DUT ZR
        3. CTH ZC sends Configure Reporting ZCL Command to DUT ZR
               Command: Configure Reporting (0x06)
               Reporting configuration record:
                      Direction: Reported (0x00)
                      Attribute: OnOff (0x0000)
                      Data Type: Boolean (0x10)
               Minimum Interval: 0
               Maximum Interval: 65534

        4. CTH ZC sends Toggle ZCL Command to DUT ZR

Expected outcome:

	1. CTH ZC creates a network

	2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by CTH ZC

    3. CTH ZC successfully binds to DUT ZR On/OFF Cluster

    4. (Test procedure 3) DUT ZR responds with Success status on Configure Reporting ZCL command
               Command: Configure Reporting Response (0x07)
               Status: Success (0x0)

	5. DUT ZR send Report Attributes ZCL command
               Attribute: OnOff (0x0000)
               Data Type: Boolean (0x10)
               On/off Control: On (0x01)

    6. (Test procedure 4) DUT ZR responds with Default Response on Toggle Command from CTH ZC
               Command: Default Response (0x0b)
               Status: Success (0x00)

        7. DUT ZR send Report Attributes ZCL command
               Attribute: OnOff (0x0000)
               Data Type: Boolean (0x10)
               On/off Control: Off (0x00)
