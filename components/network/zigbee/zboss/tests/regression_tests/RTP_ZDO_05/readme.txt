Bug ZOI-69 - Binding to the wrong cluster is possible
RTP_ZDO_05 - A device will send Bind Response with Success status in case of Bind Request for undeclared cluster

Bug notes:

    The Zigbee R22 specification does not describe stack behavior in case of receiving Bind Request with
        nonexistent or undeclared Cluster Id.

    It was decided to allow such binding, so ZBOSS should create Binding Table Entries
        even for nonexistent clusters.

Objective:

    To confirm that a device will create Bind Table Entry in case of Bind Request for undeclared or nonexistent cluster
        and that a device will be able to remove such entry by Unbind Request

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED receive Bind Request for existing On/Off cluster and respond with Success status
    4. Wait for DUT ZED receive Bind Request for unknown/invalid cluster (0x1234) and respond with Success status
    5. Wait for DUT ZED receive Bind Request for undeclared Color Control cluster and respond with Success status
    6. Wait for DUT ZED receive Binding Table Request and respond with Binding Table Response with first two entries
    7. Wait for DUT ZED receive Binding Table Request and respond with Binding Table Response with one remaining entry
    8. Wait for DUT ZED receive Unbind Request for undeclared Color Control cluster and respond with Success status
    9. Wait for DUT ZED receive Unbind Request for unknown/invalid cluster (0x1234) and respond with Success status
    10. Wait for DUT ZED receive Unbind Request for existing On/Off cluster and respond with Success status
    11. Wait for DUT ZED receive Binding Table Request and respond with Binding Table Response without any binding entries

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3.1. DUT ZED receives Bind Request for existing On/Off cluster and responds with Bind Response with "Success" status
    3.2. DUT ZED receives Bind Request for unknown/invalid cluster (0x1234) and responds with Bind Response with "Success" status
    3.3. DUT ZED receives Bind Request for undeclared Color Control cluster and responds with Bind Response with "Success" status

    4.1. DUT ZED receives Binding Table Request and responds with Binding Table Response with the following data:
        ZigBee Device Profile, Binding Table Response, Status: Success
            Status: Success (0)
            Table Size: 3
            Index: 0
            Table Count: 2
            Binding Table
                Bind
                    Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                    Source Endpoint: 143
                    Cluster: 0x0006
                    Destination: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                    Destination Endpoint: 10
                Bind
                    Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                    Source Endpoint: 143
                    Cluster: 0x1234
                    Destination: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                    Destination Endpoint: 10

    4.2. DUT ZED receives Binding Table Request and responds with Binding Table Response with the following data:
        ZigBee Device Profile, Binding Table Response, Status: Success
            Status: Success (0)
            Table Size: 3
            Index: 2
            Table Count: 1
            Binding Table
                Bind
                    Source: 00:00:00_00:00:00:00:01 (00:00:00:00:00:00:00:01)
                    Source Endpoint: 143
                    Cluster: 0x0300
                    Destination: aa:aa:aa:aa:aa:aa:aa:aa (aa:aa:aa:aa:aa:aa:aa:aa)
                    Destination Endpoint: 10

    5.1. DUT ZED receives Unbind Request for undeclared Color Control cluster and responds with Unbind Response with "Success" status
    5.2. DUT ZED receives Unbind Request for unknown/invalid cluster (0x1234) and responds with Unbind Response with "Success" status
    5.3. DUT ZED receives Unbind Request for existing On/Off cluster and responds with Unbind Response with "Success" status

    6. DUT ZED receives Binding Table Request and responds with Binding Table Response without any binding entries
        ZigBee Device Profile, Binding Table Response, Status: Success
            Status: Success (0)
            Table Size: 0
            Index: 0
            Table Count: 0
