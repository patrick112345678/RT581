Bug 14647 - Identify Time attribute update does not trigger identification 
RTP_ZCL_04 - Start identification procedure after writing of the IdentifyTime attribute

Objective:

	To confirm that the device will start identification procedure after receiving command to write the IdentifyTime attribute with values other than 0x0000.

Devices:

	1. DUT - ZC
	2. TH  - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
    2. Power on TH ZR
    3. Wait for DUT receive ZCL Identify Query and send default response
	4. Wait for DUT receive ZCL write IdentifyTime attribute request and send write response
	5. Wait for DUT receive ZCL identify Query and send Identify Query Response 

Expected outcome:

	1. ZC creates a network

	2. ZR starts bdb_top_level_commissioning and gets on the network established by ZC

	3. DUT ZC receives ZCL Identify Query command and sends default response with Success (0x00) status

	4. DUT ZC receives ZCL write attribute request for IdentifyTime:
               Attribute: IdentifyTime (0x0000)
               Data Type: 16-Bit Unsigned Integer (0x21)
               On Time: 10 seconds

	Then DUT ZC sends ZCL write attribute response with Success (0x00) status

	5. After approximately 3 seconds DUT ZC receives ZCL Identify Query command, then sends ZCL Identify Query Response:
		Identify Timeout should be approximately equal to 7
