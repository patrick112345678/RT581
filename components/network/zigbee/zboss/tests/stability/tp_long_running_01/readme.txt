ZOI-579 - Perform long running and load testing
TP_LONG_RUNNING_01 - network can stand long time without getting stuck and without crashes

Objective:

    To confirm that network can stand long time and stay workable

Devices:

    1. ZC
    2. ZR
    3. ZED

Initial conditions:

    1. All devices are factory new and powered off until used.
    2. Max children count is set to 1 for ZC to prevent direct ZED connection.
        Devices should be connected according to the following scheme: ZC <- ZR <- ZED.

Test procedure:

    1. Power on ZC
    2. Power on ZR, wait for ZR association with ZC
    3. Power on ZED, wait for ZED association with ZR

    4.1. Wait for ZED start to send On/Off On to ZC every 5 seconds
    4.2. Wait for ZED start to send Match Descriptor Request to ZR every 5 seconds
    4.3. Wait for ZR start to send On/Off Off to ZC every 5 seconds
    4.4. Wait for ZC start to send Active EP Request to ZED every 5 seconds

    5. Wait for a long time, for example, 1 day or even 1 week to confirm that network will be staying alive during
        the test procedure.

Expected outcome:

    1. ZR starts bdb_top_level_commissioning and gets on the network established by ZC
    2. ZED starts bdb_top_level_commissioning and gets on the network established by ZC through ZR

    3. ZR performs Finding & Binding as initiator with ZC and starts to send On/Off Off to ZC every 5 seconds
    4. ZED performs Finding & Binding as initiator with ZC and starts to send On/Off On to ZC every 5 seconds

    5. ZC starts to send Active EP Request to ZED every 5 seconds
    6. ZED starts to send Match Descriptor Request to ZR every 5 seconds

    7.1. Devices should continue to send packets according to E.O. steps 3, 4, 5, 6 during the test procedure
    7.2. Devices should not crash or getting stuck during the test procedure
