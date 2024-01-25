Bug 11074 - ZBOSS timer should not rely on default values 
RTP_OSIF_03 - ZBOSS should store and use own default timer parameters

Objective:

    To confirm that the ZBOSS store and use own default timer parameters

Devices:

    1. DUT - ZC

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Wait for DUT ZC print default timer parameters to the trace 

Expected outcome:

    1. ZC starts successfully

    2. The DUT ZC writes the following string in the trace: 
        - Default timer config: freq 4, mode 0, bit_width 3, int_priority 6, context used 0