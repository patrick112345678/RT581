Bug 14939 - ZBOSS APS selfie frames cause memory leak
RTP_APS_01 - memory leak in zb_aps_pass_local_msg_up()

Objective:

    To confirm that selfie frames are correctly handled by zb_aps_pass_local_msg_up().

Devices:

    1. DUT - ZC

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Wait for 5 seconds

Expected outcome:

    1. There is trace msg in DUT ZC trace: "mgmt_bind_resp_cb: retrieved all entries - 16"

    2. There is not trace msg in DUT ZC trace: "mgmt_bind_resp_cb: TEST_FAILED"
