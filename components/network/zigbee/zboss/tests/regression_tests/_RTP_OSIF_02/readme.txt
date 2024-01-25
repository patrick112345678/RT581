Bug 13795 - OSIF'ying the TRACE_MSG 
RTP_OSIF_02 - The zb_trace_msg_port_do() function is overrode and handles trace messages

Objective:

    To confirm that there is ability to override zb_trace_msg_port_do() function and process messages data

Devices:

    1. DUT - ZC

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Wait for DUT ZC write test string to the trace

Expected outcome:

    1. ZC starts successfully

    2. The DUT ZC writes the following strings in the trace (at least one time): 
        - Test trace message, test arg 29
        - Test trace message, test arg 196