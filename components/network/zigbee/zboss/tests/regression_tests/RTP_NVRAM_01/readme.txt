Nordic stabilization, task 71248 - Power cycle device during continuous NVRAM writing, check if datasets are correct 
RTP_NVRAM_01 - device loads NVRAM after power off and on during continuous NVRAM writing

Objective:

	To confirm that the device can load NVRAM datasets after power off and on during continuous NVRAM writing

Devices:

	1. DUT - ZC1
	2. DUT - ZC2 
	3. TH - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. DUT ZC1 and DUT ZC2 presents one split device (to simulate the test behaviour). 
	   They should be run on the same physical device and should use the same NVRAM.
	3. DUT ZC2 should not erase NVRAM at start

Test procedure:

	1. Power on DUT ZC1
	2. Wait for DUT ZC1 crash during continuous NVRAM writing
	3. Power on DUT ZC2
	4. Power on TH ZR
	5. Wait for DUT ZC2 receive Buffer Test Request
	6. Wait for DUT ZC2 respond with Buffer Test Response

Expected outcome:

	1. DUT ZC1 creates a network

	2. DUT ZC1 tries to write APP1 dataset to NVRAM and crashes.
	
	3.1. DUT ZC1 prints to trace the string "Trying to write nvram APP_DATA1 dataset, len X" (X - some integer, length of the dataset)
	3.1. DUT ZC1 prints to trace the string "Aborted on nvram write, written len Y" (Y - some integer, length of the written part of the dataset)
		The length Y must be lesser than the length X from step 3.1.

	4. DUT ZC2 successfully loads NVRAM of DUT ZC1

	5. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC2

	6. DUT ZC2 receives Buffer Test Request

	7. DUT ZC2 responds with Buffer Test Response

	8. The trace should not contain the following string: "Read nvram APP_DATA1 payload".