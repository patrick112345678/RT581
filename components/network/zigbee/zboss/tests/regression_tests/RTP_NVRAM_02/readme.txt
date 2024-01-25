Nordic stabilization, task 71249 - Calculate and force dataset write sequence to force migration in the middle of dataset writing
RTP_NVRAM_02 - device performs NVRAM migration in the middle of dataset writing

Objective:

	To confirm that the device can complete NVRAM migration in the middle of dataset writing

Devices:

	1. DUT - ZC
	2. TH - ZR

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. DUT ZC should not erase NVRAM at start

Test procedure:

	1. Power on DUT ZC
	2. Power on TH ZR
	3. Wait for DUT ZC and TH ZR association
	4. Wait for DUT ZC perform migration due to page overflow by old datasets
	5. Power off DUT ZC
	6. Power on DUT ZC
	7. Wait for DUT ZC receive Buffer Test Request
	8. Wait for DUT ZC respond with Buffer Test Response (to confirm that device can continue to work after NVRAM migration)

Expected outcome:

	1. DUT ZC creates a network

	2. TH ZR start bdb_top_level_commissioning and get on the network established by TH ZC

	3. DUT ZC writes APP1 NVRAM dataset 15 times to fill current NVRAM page and trigger migration process.

	The trace should contain the following strings:
		- test_nvram_write_app_data, page 0, pos X (this string should be repeated 14 times, X - some integer)
		- test_nvram_write_app_data, page 1, pos X (this string should be repeated 1 times, X - some integer)

	4. DUT ZC restores datasets from NVRAM after power off and on

	The trace should contain the following strings:
		- test_nvram_read_app_data page 1 pos X (X - integer that should be equal to last writing pos)
		- test nvram app data validation: 1

	5. DUT ZC receives LQI Request

	6. DUT ZC responds with LQI Response