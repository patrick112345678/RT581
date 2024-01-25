Nordic stabilization, task 71258 -  Check TX power value boundaries
RTP_PRODCONF_03 - device will correctly handles boundary and inappropriate TX power parameters from Production Config

Objective:

	To confirm that the device can correctly handle boundary and inappropriate TX power parameters from Production Config

Devices:

	1. DUT - ZC

Initial conditions:

	1. All devices are factory new and powered off until used.
	2. DUT ZC should be flashed and started with Production Configs in the following order: 
		- rtp_prodconf_03_dut_zc_1.txt
		- rtp_prodconf_03_dut_zc_2.txt
		- rtp_prodconf_03_dut_zc_3.txt

Test procedure:

	1. Power on DUT ZC with rtp_prodconf_03_dut_zc_1.txt
	2. Wait for DUT ZC load and apply TX power parameters from Production Config

	3. Power on DUT ZC with rtp_prodconf_03_dut_zc_2.txt
	4. Wait for DUT ZC load and apply TX power parameters from Production Config

	4. Power on DUT ZC with rtp_prodconf_03_dut_zc_3.txt
	5. Wait for DUT ZC load and apply TX power parameters from Production Config

Expected outcome:

	1. DUT ZC loads and applies TX power parameters from Production Config. The trace should contain the following strings:
		- signal: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, status 0
		- tx_power[default]: -50
		- tx_power[current]: -40
		- tx_power[current_nrf]: -40

	2. DUT ZC loads and applies TX power parameters from Production Config. The trace should contain the following strings:
		- signal: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, status 0
		- tx_power[default]: -5
		- tx_power[current]: -4
		- tx_power[current_nrf]: -4

	3. DUT ZC loads and applies TX power parameters from Production Config. The trace should contain the following strings:
		- signal: ZB_ZDO_SIGNAL_PRODUCTION_CONFIG_READY, status 0
		- tx_power[default]: 50
		- tx_power[current]: 8
		- tx_power[current_nrf]: 8