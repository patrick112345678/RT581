Bug 10859 - Disabling ZLL compatibility causes ZB_ABORT() at the On/Off attribute handling 
RTP_ZCL_15 - Device with on-off cluster with only mandatory attributes and with disabled ZLL processes on-off commands

Objective:

       To confirm that device device with on-off cluster with only mandatory attributes and with disabled ZLL can process on-off commands

Devices:

       1. TH - ZC
       2. DUT - ZED

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. DUT ZED has On Off cluster with only mandatory attribute OnOff
       3. OnOff attribute initial value is Off (0x00) 
       4. All devices must be built with vendor zb_vendor_cfg_nsng_regression_autotests_ll.h or with zb_vendor_cfg_nrf52840_regression_autotests_ll.h

Test procedure:

       1. Power on TH ZC
       2. Power on DUT ZED
       3. Wait for TH ZC bind with DUT ZED
       4. Wait for DUT ZED receive OnOff cluster On command and responds with the Default Response
       5. Wait for DUT ZED receive Read Attributes Request for OnOff attribute and responds with the Read Attributes Response
       6. Wait for DUT ZED receive OnOff cluster Off command and responds with the Default Response
       7. Wait for DUT ZED receive Read Attributes Request for OnOff attribute and responds with the Read Attributes Response

Expected outcome:

       1. TH ZC creates a network

       2. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. TH ZC starts finding and binding procedure as initiator and binds with DUT ZED

       4.1. DUT ZED receive OnOff cluster On command from TH ZC
       4.2. DUT ZED responds with the Default Response with Success (0x00) status

       5.1. DUT ZED receives Read Attributes Request from TH ZC:
              Command: Read Attributes (0x00)
              Attribute: OnOff (0x0000)
       5.2. DUT ZED responds with the Read Attributes Response:
              Attribute: OnOff (0x0000)
              Status: Success (0x00)
              On/off Control: On (0x01)

       6.1. DUT ZED receive OnOff cluster Off command from TH ZC
       6.2. DUT ZED responds with the Default Response with Success (0x00) status

       7.1. DUT ZED receives Read Attributes Request from TH ZC:
              Command: Read Attributes (0x00)
              Attribute: OnOff (0x0000)
       7.2. DUT ZED responds with the Read Attributes Response:
              Attribute: OnOff (0x0000)
              Status: Success (0x00)
              On/off Control: Off (0x00)