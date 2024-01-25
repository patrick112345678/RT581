Bug 14615 - No long address resolution for binding table entries
RTP_ZDO_02 - Address Discovery after 1st APS delivery fail

Objective:

        Confirm that callback which is set by zb_af_set_data_indication() launches before stack handling of ZDO and ZCL frames.

Devices:

	1. TH  - ZC
	2. TH  - ZR
        3. DUT - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1.  Power on TH ZC
        2.  Power on TH ZR
        3.  Power on DUT ZED
        4.  DUT ZED sends On/Off On Command
        5.  DUT ZED sends On/Off Off Command
        6.  Power off DUT ZED
        7.  TH ZR changes it's short address and sends Device Announcement
        8.  Power on DUT ZED
        9.  DUT ZED sends On/Off On Command
        10. DUT ZED sends On/Off Off Command

Expected outcome:

	1. TH ZC creates a network

	2. TH ZR successfully associates to TH ZC

	3. DUT ZED successfully associates to TH ZC

        4. DUT ZED sends On/Off On Command to TH ZR
           Cluster: On/Off (0x0006)
           Command: On (0x01)

        5. DUT ZED sends On/Off Off Command to TH ZR
           Cluster: On/Off (0x0006)
           Command: Off (0x00)

        6. While DUT ZED is powered off (Test procedure - 6) TH ZR internally changes it's short address (Test procedure - 7) and sends Device Announcement using the new short address
           Device Announcement (Cluster ID: 0x0013)
           Extended Address: 00:00:00_01:00:00:00:01 (00:00:00:01:00:00:00:01)

        7. After DUT ZED is powered on (Test Procedure - 8) it rejoins to TH ZC
           Destination: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
           Extended Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
           Command Identifier: Rejoin Request (0x06)

        8.1 DUT ZED sends On/Off On Command to the TH ZR old short address (to the address that was before Test procedure - 7)
            Source: [DUT ZED short NWK address]
            Destination: [TH ZR old short NWK address]
            Destination: 00:00:00_01:00:00:00:01 (00:00:00:01:00:00:00:01)
            Extended Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                     Cluster: On/Off (0x0006)
                     Command: On (0x01)
        8.2 TH ZR does not acknowledge the frame (no mac ack from TH ZR)

        9.1. There is a trace message in the dut's trace: "test_send_on_off_cb: transmission failed"
        9.2. There should be only one such message in the dut's trace.

        10. DUT ZED broadcasts Network Address Request (Cluster ID: 0x0000)
            Network Address Request (Cluster ID: 0x0000)
            Extended Address: 00:00:00_01:00:00:00:01 (00:00:00:01:00:00:00:01)
            Request Type: Single Device Response (0)

        11. DUT ZED receives Network Address Response (Cluster ID: 0x8000) from TH ZR
            Status: Success (0)
            Extended Address: 00:00:00_01:00:00:00:01 (00:00:00:01:00:00:00:01)
            Nwk Addr of Interest: [TH ZR new short NWK address]

        12. DUT ZED sends On/Off Off Command to the TH ZR new short address (to the address that is set after Test procedure - 7)
            Source: [DUT ZED short NWK address]
            Destination: [TH ZR new short NWK address]
            Destination: 00:00:00_01:00:00:00:01 (00:00:00:01:00:00:00:01)
            Extended Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                     Cluster: On/Off (0x0006)
                     Command: Off (0x00)
