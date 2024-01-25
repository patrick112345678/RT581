ZOI-586 - APS User Payload feature is broken
RTP_APS_14 - the device can send APS packet with user's payload

Objective:

    To confirm that device can send APS packets with user's payload and will receive sending confirms.

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED send Link Quality Request to TH ZC
    4. Wait for DUT ZED send ZCL On/Off Toggle to TH ZC
    5. Wait for DUT ZED send APS packet with custom user's payload to TH ZC and receive APS Ack
    6. Wait for DUT ZED send APS packet with custom user's payload to TH ZC, but will not receive APS Ack
    7. Power off TH ZC
    8. Wait for DUT ZED send APS packet with custom user's payload to TH ZC

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED sends Link Quality Request to TH ZC
        The trace should contain the following strings:
            test_send_mgmt_lqi_resp_cb: success

    4. DUT ZED sends ZCL On/Off Toggle to TH ZC
        The trace should contain the following strings:
            test_send_on_off_toggle_req_cb: status 0

    5. Trace should not contain string "test_send_request_with_aps_user_payload_tx_cb" before step 6 validation.

    6.1. DUT ZED sends APS packet with custom user's payload:
        01 02 03 04 05 06 07 08 09 0a
    6.2. DUT ZED receives APS Ack for the packet from step 6.1.
        The trace should contain the following strings:
            buf_status 0, buf_len: 18, aps_payload_size: 10
            Transmission status: SUCCESS

    7.1. DUT ZED sends APS packet with custom user's payload:
        01 02 03 04 05 06 07 08 09 0a
    7.2. DUT ZED doesn't receive APS Ack for the packet from step 7.1.
        The trace should contain the following strings:
            buf_status 190, buf_len: 18, aps_payload_size: 10
            Transmission status: NO_APS_ACK
    7.3. After appearing strings from step 7.2. it is possible to stop ACK waiting in validation script.


    8.1. DUT ZED sends APS packet with custom user's payload:
        01 02 03 04 05 06 07 08 09 0a
    8.2. DUT ZED doesn't receive APS or MAC Acks for the packet from step 8.1.
        The trace should contain the following strings:
            buf_status 190, buf_len: 18, aps_payload_size: 10
            Transmission status: NO_APS_ACK
