R22 - Transmission via binding. zb_nlde_data_confirm()
RTP_APS_10 - device skips frames with status ZB_NWK_STATUS_FRAME_NOT_BUFFERED in case of binding transmission

Objective:

    To confirm that device will skip frames with status ZB_NWK_STATUS_FRAME_NOT_BUFFERED in case of binding transmission and will schedule new frames or clear binding transmission table

Devices:

    1. DUT - ZC
    2. TH - ZR1
    3. TH - ZR2

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. [APP], [APS] trace are enabled on the DUT

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR1, TH ZR2
    4. Wait for TH ZR1, TH ZR2 associate with DUT ZC
    5. Wait for DUT ZC bind with TH ZR1, TH ZR2

    6.1. Wait for DUT ZC schedule sending of custom ZCL command using binding transmission two times (with oversized and normal payload)
    6.2. Wait for DUT ZC send custom ZCL command with normal payload

    7.1. Wait for DUT ZC schedule sending of custom ZCL command using binding transmission two times (with normal and oversized payload)
    7.2. Wait for DUT ZC send custom ZCL command with normal payload

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZR1, TH ZR2 starts bdb_top_level_commissioning and gets on the network established by DUT ZC

    3. DUT ZC performs binding with TH ZR1, TH ZR2

    4.1. DUT ZC schedules sending of custom ZCL command using binding transmission two times (with oversized and normal payload).
    4.2. DUT ZC skips command with oversized payload and schedule new binding transmission.
        The trace should contain the following strings:
            - >>send_test_zcl_cmd_req, param {0} ({0} - buffer id for command with oversized payload)
            - >>send_test_zcl_cmd_req, param {1} ({1} - buffer id for command with normal payload)

            - record in retrans table for unbuffered or purged frame {0}, is_binding_trans 1
            - finally done with this data pkt i 0 status -21 (-21 - RET_PENDING status)
            - +zb_process_bind_trans {0}
            - found binding transmission entry, index %d (%d - some integer)

            - zcl command send status error, param {0}

    4.3. DUT ZC sends custom ZCL command with 65 bytes payload to TH ZR1, TH ZR2
        The trace should contain the following strings:
            - command send status success, param {0}

    5.1. DUT ZC schedules sending of custom ZCL command using binding transmission two times (with normal and oversized payload).
    5.2. DUT ZC skips command with oversized payload.
        The trace should contain the following strings:
            - >>send_test_zcl_cmd_req, param {2} ({2} - buffer id for command with normal payload)
            - >>send_test_zcl_cmd_req, param {3} ({3} - buffer id for command with oversized payload)

            - record in retrans table for unbuffered or purged frame {3}, is_binding_trans 1
            - finally done with this data pkt i 1 status -21 (-21 - RET_PENDING status)

            - zcl command send status error, param {3}

    5.3. DUT ZC sends custom ZCL command with 65 bytes payload to TH ZR1, TH ZR2
        The trace should contain the following strings:
            - command send status success, param {2}
