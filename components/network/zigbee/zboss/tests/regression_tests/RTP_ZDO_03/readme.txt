Bug 14383 - Question: Radio sleep state
RTP_ZDO_03 - Radio sleep state for Sleepy ZED

Objective:

        Confirm that transceiver is off during joining attempts and when ZB_COMMON_SIGNAL_CAN_SLEEP is received by a sleepy end device.

Devices:

	1. TH  - ZC
	2. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.
        2. [MAC1] trace is enabled on the DUT.

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZED
        3. Wait for DUT ZR Leaves the network
        4. Wait for Beacon Requests from the DUT ZED

Expected outcome:

	1. TH ZC creates a network

        2. DUT ZED associates TH ZC

        3.1. DUT ZED polls TH ZC
        3.2. There is trace msg in dut's trace:
             [APP1] signal: ZB_COMMON_SIGNAL_CAN_SLEEP, status 0
             [APP1] zb_zdo_startup_complete(): Transceiver is OFF - test OK
        3.2. There isn't trace msg in dut's trace:
             [APP1] zb_zdo_startup_complete(): Transceiver is ON - test FAILED

        4. DUT ZED Leaves the network

        5.1. DUT ZED tryes to associate to TH ZC
        5.2. There is trace msg in dut's trace:
             [MAC1] >> zb_mlme_scan_step [buffer_id] page [TEST_PAGE]
             [MAC1] zb_mac_change_channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle 0
             [MAC1] << zb_mlme_scan_step
        5.3. There is trace msg in dut's trace:
             [MAC1] >> zb_mlme_scan_step [buffer_id] page [TEST_PAGE]
             [MAC1] saving page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle 0
             [MAC1] scan confirm [buffer_id] status 0
             [MAC1] << zb_mlme_scan_step
        5.4. There is trace msg in dut's trace:
             [MAC1] >> zb_mlme_scan_step [buffer_id] page [TEST_PAGE]
             [MAC1] zb_mac_change_channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle 0
             [MAC1] << zb_mlme_scan_step
        5.5. There is trace msg in dut's trace:
             [MAC1] >> zb_mlme_scan_step [buffer_id] page [TEST_PAGE]
             [MAC1] restoring original channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle 0
             [MAC1] zb_mac_change_channel page [TEST_PAGE] channel [TEST_CHANNEL]
             [MAC1] zb_mac_change_channel: MAC_PIB().mac_rx_on_when_idle 0
             [MAC1] scan confirm [buffer_id] status 0
             [MAC1] << zb_mlme_scan_step

        6. Repeat steps 5.4 - 5.5 three times
