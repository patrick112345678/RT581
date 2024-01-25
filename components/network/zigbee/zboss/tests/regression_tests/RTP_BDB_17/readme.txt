RTP_BDB_17 - ZB_ZDO_SIGNAL_DEVICE_UPDATE and ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signals after ZDO MGMT Leave Request

Objective:

	To confirm that ZB_ZDO_SIGNAL_DEVICE_UPDATE and ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signals are received by the application and have correct params after ZDO MGMT Leave request is received

Devices:

	1. DUT - ZC
	2. TH  - ZR1
	3. TH  - ZED1

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on DUT ZC
        2. Power on TH ZR1
        3. Power on TH ZED1

Expected outcome:

	1. DUT ZC creates a network

        2.1 TH ZR1 successfully associates to DUT ZC
        2.2 There is trace msg in dut's trace:
            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 0.0.0.1.0.0.0.1
            [APP3] short_addr: TH_ZR, {{ZR_SHORT_ADDR}}
            [APP3] status: 0x1 - UNSECURED_JOIN
        2.3 There isn't trace msg in dut's trace with another status for device with TH ZR1 long address
        2.4 There is trace msg in dut's trace:
            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED
            [APP3] long_addr: 0.0.0.1.0.0.0.1
            [APP3] short_addr: TH_ZR, {{ZR_SHORT_ADDR}}
            [APP3] auth_type: 0x1 - R21 TCLK
            [APP3] auth_status: 0x0 - SUCCESS
        2.5 There isn't trace msg in dut's trace with another status for device with TH ZR1 long address

        3.1 TH ZED1 successfully associates to TH ZR1
        3.2 There is trace msgs in dut's trace:
            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 0.0.0.0.0.0.0.2
            [APP3] short_addr: TH_ZED, {{ZED_SHORT_ADDR}}
            [APP3] status: 0x1 - UNSECURED_JOIN
        3.3 There isn't trace msg in dut's trace with another status for device with TH ZED1 long address
        3.4 There is trace msg in dut's trace:
            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED
            [APP3] long_addr: 0.0.0.0.0.0.0.2
            [APP3] short_addr: TH_ZED, {{ZED_SHORT_ADDR}}
            [APP3] auth_type: 0x1 - R21 TCLK
            [APP3] auth_status: 0x0 - SUCCESS
        3.5 There isn't trace msg in dut's trace with another status for device with TH ZED1 long address

        4.1 DUT ZC sends ZDO Leave Req to TH ZED1:
            .0.. .... = Remove Children: False
            1... .... = Rejoin: True
        4.2 There is trace msg in dut's trace:
            [APP1] >>test_send_leave: buf_param = %hd
            [APP1] <<test_send_leave

	    [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
	    [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 00:00:00:00:00:00:00:02
	    [APP3] short_addr: TH_ZED, {{ZED_SHORT_ADDR}}
            [APP3] status: 0x0 - SECURED_REJOIN

        5.1 DUT ZC sends ZDO Leave Req to TH ZED1:
            .0.. .... = Remove Children: False
            0... .... = Rejoin: False
        5.2 There is trace msg in dut's trace:
            [APP1] >>test_send_leave: buf_param = %hd
            [APP1] <<test_send_leave

            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
            [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 00:00:00:00:00:00:00:02
            [APP3] short_addr: TH_ZED, {{ZED_SHORT_ADDR}}
            [APP3] status: 0x2 - DEVICE_LEFT

        6.1 DUT ZC sends ZDO Leave Req to TH ZR1:
            .0.. .... = Remove Children: False
            1... .... = Rejoin: True
        6.2 There is trace msg in dut's trace:
            [APP1] >>test_send_leave: buf_param = %hd
            [APP1] <<test_send_leave

            [APP1] signal: ZB_ZDO_SIGNAL_DEVICE_UPDATE, status 0
	        [APP3] ZB_ZDO_SIGNAL_DEVICE_UPDATE
            [APP3] long_addr: 00:00:00:01:00:00:00:01
	    [APP3] short_addr: TH_ZR, {{ZR_SHORT_ADDR}}
            [APP3] status: 0x0 - SECURED_REJOIN

        7.1 DUT ZC sends ZDO Leave Req to TH ZR1:
            .0.. .... = Remove Children: False
            0... .... = Rejoin: False
        7.2 There is trace msg in dut's trace:
            [APP1] >>test_send_leave: buf_param = %hd
            [APP1] <<test_send_leave
        7.3 There is no ZB_ZDO_SIGNAL_DEVICE_UPDATE or ZB_ZDO_SIGNAL_DEVICE_AUTHORIZED signal received by DUT
