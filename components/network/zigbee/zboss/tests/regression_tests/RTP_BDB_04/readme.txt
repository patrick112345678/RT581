Bug 14537 - Adding a signal ZB_BDB_DEVICE_AUTHORIZED to zboss_signal_handler
RTP_BDB_04 - ZB_ZDO_SIGNAL_DEVICE_UPDATE and ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signals

Objective:

	To confirm that ZB_ZDO_SIGNAL_DEVICE_UPDATE and ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signals are received by the application and have correct params

Devices:

	1. DUT - ZC
	2. TH  - ZR1
	3. TH  - ZR2
	4. TH  - ZR3
	5. TH  - ZR4
	6. TH  - ZR5

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR1
        3. Power on TH ZR2
        4. Power on TH ZR3
        5. Power on TH ZR4
        6. Power on TH ZR5

Expected outcome:

	1. DUT ZC creates a network

        2.1 TH ZR1 successfully associates to DUT ZC
        2.2 There is trace msg in dut's trace:
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 0.0.0.1.0.0.0.1
            [APP3] status: 0x1 - UNSECURED_JOIN
        2.3 There isn't trace msg in dut's trace with another status for device with TH ZR1 long address
        2.4 There is trace msg in dut's trace:
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED
            [APP3] long_addr: 0.0.0.1.0.0.0.1
            [APP3] auth_type: 0x1 - R21 TCLK
            [APP3] auth_status: 0x0 - SUCCESS
        2.5 There isn't trace msg in dut's trace with another status for device with TH ZR1 long address

        3.1 TH ZR2 does not associates to DUT ZC
        3.2 There is trace msgs in dut's trace:
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 0.0.0.1.0.0.0.2
            [APP3] status: 0x1 - UNSECURED_JOIN
        3.3 There isn't trace msg in dut's trace with another status for device with TH ZR2 long address
        3.4 There is trace msg in dut's trace:
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED
            [APP3] long_addr: 0.0.0.1.0.0.0.2
            [APP3] auth_type: 0x1 - R21 TCLK
            [APP3] auth_status: 0x2 - FAILED
        3.5 There isn't trace msg in dut's trace with another status for device with TH ZR2 long address

        4. Repeat steps 3.1 - 3.5 for TH ZR3 - TH ZR5 devices with short addr: 0x1003 - 0x1005

        5. There is no trace msgs with not NULL status in dut's trace
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status [status]
            [APS1] signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status [status]
