Bug ZOI-Internal-81398 (Celoxis) - BDB status not cleaned by Leave or Factory Reset after F&B target start

RTP_BDB_18 - A device will be able to perform commissioning after leaving with active Finding & Binding target process

Objective:

    To confirm that it is possible to join to network after leaving with active Finding & Binding target process

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED join to TH ZC network
    4. Wait for DUT ZED start Finding & Binding target procedure
    5. Wait for DUT ZED leave network
    6. Wait for DUT ZED join to TH ZC network

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED starts Finding & Binding target procedure
       The trace should contain the string "Trigger F&B target"

    4. DUT ZED send Leave command and leaves the network

    5. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    6. DUT ZED does not assert, crash or get stuck during the test procedure
