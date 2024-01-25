Bug ZOI-44 - ZCL Occupancy Sensing cluster implementation seems to be missing 
RTP_ZCL_17 - Zigbee Occupancy cluster attribute setting gives Invalid Value error

Objective:

    To confirm that it is possible to set Occupancy Sensing cluster attribute values and configure reporting to notify about change of attributes' values

Devices:

    1. TH - ZC
    2. DUT - ZED

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on TH ZC
    2. Power on DUT ZED
    3. Wait for DUT ZED send ZCL Report Attributes command with "occupied" status
    4. Wait for DUT ZED send ZCL Report Attributes command with "unoccupied" status

Expected outcome:

    1. TH ZC creates a network

    2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

    3. DUT ZED sends ZCL Report Attributes command with "occupied" status
        Attribute Field
            Attribute: Occupancy (0x0000)
            Data Type: 8-Bit Bitmap (0x18)
            Occupancy: 0x01, Occupied
                .... ...1 = Occupied: True

    4. DUT ZED sends ZCL Report Attributes command with "occupied" status
        Attribute Field
            Attribute: Occupancy (0x0000)
            Data Type: 8-Bit Bitmap (0x18)
            Occupancy: 0x00
                .... ...0 = Occupied: False

    5. There should not be other Report Attributes commands except commands from steps 3 and 4.

    6. The DUT ZED device do not crash or have assertion error
