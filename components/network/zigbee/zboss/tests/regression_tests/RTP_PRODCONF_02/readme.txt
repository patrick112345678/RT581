Nordic stabilization, task 71257 - Modify IEEE, TX power, channel mask, Manufacturer, Device Model, Install code via Prod config, check if applied 
RTP_PRODCONF_02 - device uses Production Config to initialize TX power parameters

Objective:

	To confirm that the device can load and apply TX power parameters from Production Config

Devices:

	1. DUT - ZC

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. DUT ZC uses Production Config with parameters from rtp_prodconfig_02_dut_zc.txt

Test procedure:

	1. Power on DUT ZC
	2. Wait for DUT ZC load and apply TX power parameters from Production Config

Expected outcome:

	1. DUT ZC loads and applies TX power parameters from Production Config. The trace should contain the following strings:
		- signal: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, status 0
		- tx_power[default]: 2
		- tx_power[current]: 2
		- tx_power[pg=P,ch=C]: X
			P - number of page from 0 to 4
			C - number of channel
			X - TX power value for page P and channel C, the following values are allowed: -40, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8 (nrf_802154_pib.c)

		TX power values should satisfy the following requirements:
			- values for all channels of page 0 (except channel 15) should be equal to 2
			- value for page 0, channel 15 should be equal to 7

			- values for all channels of page 1 (except channel 0) should be equal to 3
			- value for page 1, channel 0 should be equal to 8

			- values for all channels of page 2 (except channels 27, 34, 62) should be equal to 4
			- value for page 2, channel 27 should be equal to -40
			- value for page 2, channel 34 should be equal to -20
			- value for page 2, channel 62 should be equal to -16

			- values for all channels of page 3 (except channels 35, 61) should be equal to 5
			- value for page 3, channel 35 should be equal to -12
			- value for page 3, channel 61 should be equal to -8

			- values for all channels of page 4 (except channel 0) should be equal to 6
			- value for page 4, channel 0 should be equal to -4