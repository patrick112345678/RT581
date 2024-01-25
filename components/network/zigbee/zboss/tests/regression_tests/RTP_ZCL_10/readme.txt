Bug 12721 - Unable to send ZCL frame, which contains 64 or 65 bytes 
RTP_ZCL_10 - It is possible to send ZCL frame, which contains 64 or 65 bytes 

Objective:

    To confirm that the device can send ZCL frame, which contains 64 or 65 bytes 

Devices:

	1. DUT - ZC
	2. TH - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
    2. Power on TH ZR
    3. Wait for TH ZR and DUT ZC complete finding and binding procedure
	4. Wait for DUT ZC send custom ZCL command with 64 bytes payload to TH ZR and receive Default Response
	5. Wait for DUT ZC send custom ZCL command with 65 bytes payload to TH ZR and receive Default Response 

Expected outcome:

	1. DUT ZC creates a network

	2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC

	3. DUT ZC performs finding and binding (DUT ZC as initiator, TH ZR as target):
       DUT ZC binds to the Color Control cluster of TH ZR.

	4.1. DUT ZC sends to TH ZR custom ZCL command from custom cluster with 64 bytes payload (64 integers from 0 to 63, command ID is 0xAA)
	4.2. DUT ZC receives Default Response with Success status

	5.1. DUT ZC sends to TH ZR custom ZCL command from custom cluster with 65 bytes payload (65 integers from 0 to 64, command ID is 0xAA)
	5.2. DUT ZC receives Default Response with Success status