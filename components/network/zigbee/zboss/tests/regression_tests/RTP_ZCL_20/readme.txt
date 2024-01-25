ZOI-95 - An issue with reporting between two local EPs
RTP_ZCL_20 - It is possible to use reporting between local endpoints

Objective:

    To confirm that the device will send reports to bound local endpoints

Devices:

    1. DUT - ZC
    2. TH - ZR

Initial conditions:

    1. All devices are factory new and powered off until used.

Test procedure:

    1. Power on DUT ZC
    2. Power on TH ZR
    3.1. Wait for DUT ZC send ZCL Report Attributes command with On/Off attribute to TH ZR
    3.2. Wait for DUT ZC send ZCL Report Attributes command with Current Level attribute to TH ZR
    4. Wait for DUT ZC print messages about receiving local reports

Expected outcome:

    1. DUT ZC creates a network

    2. TH ZR starts bdb_top_level_commissioning and gets on the network established by DUT ZC

    3.1. DUT ZC sends ZCL Report Attributes command with On/Off attribute to TH ZR
        Attribute Field
            Attribute: OnOff (0x0000)
            Data Type: Boolean (0x10)
            On/off Control: Off (0x00)

    3.2. DUT ZC sends ZCL Report Attributes command with Current level attribute to TH ZR
        Attribute Field
            Attribute: Current Level (0x0000)
            Data Type: 8-Bit Unsigned Integer (0x20)
            Current Level: 1

    4. The DUT ZC' trace should contain the following messages in specified order:

        - On/Off report is received, dst_ep 10, new_value 0
        - On/Off report is received, dst_ep 11, new_value 0
        - Current Level report is received, dst_ep 10, new_value 1
        - Current Level report is received, dst_ep 11, new_value 1

    5. The DUT ZC should not crash or fail with assertion during the test procedure.
