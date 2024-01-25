R22 - Light switch and several bulb devices 
RTP_APS_06 - a device can handle network loss during APS transmission

Objective:

       To confirm that a ZBOSS device can handle network loss during APS transmission

Devices:

       1. TH - ZC
       2. DUT - ZED (light switch)
       3. TH - ZR1 (bulb)
       4. TH - ZR2 (bulb)
       5. TH - ZR3 (bulb)

Initial conditions:

       1. All devices are factory new and powered off until used.
       2. [APP], [APS] trace are enabled on the DUT
       3. Max children count is set to 3 for TH_ZC, to 1 for TH ZR1, and to 0 for TH ZR2, TH ZR3, 
          to ensure that DUT ZED will join to network as TH ZR1 child)

Test procedure:

       1. Power on TH ZC
       2. Power on TH ZR2, TH ZR3
       3. Power on TH ZR1
       4. Wait for TH ZR1, TH ZR2, TH ZR3 associate with TH ZC
       3. Power on DUT ZED
       5. Wait for DUT ZED associate with TH ZR1
       6. Wait for DUT ZED lose network during APS transmission

Expected outcome:

       1. TH ZC creates a network

       2. TH ZR1, TH ZR2, TH ZR3 starts bdb_top_level_commissioning and gets on the network established by TH ZC

       3. DUT ZED starts bdb_top_level_commissioning and gets on the network established by TH ZR1

       4. The trace should contains the following string:
              - Got -962 status from NWK - confirm APS packet send failure now

       5. The DUT ZED does not fail with ASSERT during the test procedure