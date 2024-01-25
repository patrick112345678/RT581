Bug ZOI-Internal-81321 (Celoxis) - Closed network permits new devices after device leave

RTP_NWK_04 - Closed network will permit association of new device after any device leave

Objective:

    To confirm that a device will not open network after leaving of child device

Devices:

    1. TH - ZC
    2. DUT - ZR
    3. TH - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZR
    3. Wait for DUT ZR association with TH ZC
    4. Power on TH ZED
    5. Wait for TH ZED association with DUT ZR
    6. Wait for 180 seconds until TH ZC will close the network
    7. Wait for TH ZED leaves the network
    8. Restart TH ZED
    9. Wait for TH ZED unsuccessful association attempt

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZR starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. TH ZED starts bdb_top_level_commissioning and associates with DUT ZR

    4. TH ZC close network after Permit Join timeout

    5. TH ZED leaves network after Permit Join timeout

    6. TH ZED tries to join to the network, sends Beacon Request and receive Beacons
      from TH ZC and DUT ZR with false Association Permit flag
        0... .... .... .... = Association Permit: False

      i.e. the network should be closed and TH ZED should not join to it