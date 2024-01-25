Bug 11835 - Association failed due to retransmission errors
RTP_SEC_01 - TC link key through router node

Objective:

	To confirm that the router successfully forwards packets to end device from TC.

Devices:

	1. DUT - ZR
	2. TH  - ZC
	3. TH  - ZED

Initial conditions:

	1. All devices are factory new and powered off until used.

Test procedure:

	1. Power on TH ZC
	2. Power on DUT ZR
	3. Power on TH ZED
	4. Wait for TH ZED associates with DUT ZR
        5. TH ZC sends LQI Request to DUT ZR

Expected outcome:

	1. TH ZC creates a network

	2. DUT ZR successfully joined and authorized with TH ZC

        3. DUT ZR forwards Request Key, Transport Key, Verify Key and Confirm Key while TH ZED's authorization in the network

        4. (Test procedure 5) DUT ZR responds to TH ZC with LQI Response
           Status: Success (0)
           Table Size: 2
           Table Count: 2

           Neighbor table:
               Table entry:
                   Extended Pan: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                   Extended Address: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                   Addr: 0x0000
                   .... ..00 = Type: Coordinator (0)
                   .... 01.. = Idle Rx: True (1)
                   .000 .... = Relationship: Parent (0)
                   .... ..01 = Permit Joining: True (1)
                   Depth: 0
               Table entry:
                   Extended Pan: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                   Extended Address: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                   Addr: [ZED short address]
                   .... ..10 = Type: End Device (2)
                   .... 01.. = Idle Rx: True (1)
                   .001 .... = Relationship: Child (1)
                   .... ..00 = Permit Joining: False (0)
                   Depth: 2
