Bug 11974 - Sleeping cannot be interrupted by any event
RTP_INT_03 - There should be ability to wake up end-device by some external event

Objective:

    To confirm that the interrupt from some external event will wake up the device

Devices:

    1. TH  - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. The pin P1.11 of DUT ZED connected to the pin P1.13 of TH ZC

Test procedure:

    1. Power on DUT ZED
    2. Power on TH ZC
    3. Wait for DUT ZED enter to the sleep mode
    4. Wait for DUT ZED wake up by external signal from TH ZC
    5. Wait for DUT ZED send Buffer Test Request to TH ZC

Expected outcome:

    1. TH ZC and DUT ZED starts successfully

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. The DUT ZED trace should contain the string "Waking up test OK".

    4. The DUT ZED does not fail with ASSERT during the test procedure.
