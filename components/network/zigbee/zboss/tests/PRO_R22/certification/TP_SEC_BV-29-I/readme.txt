11.29	TP/SEC/BV-29-I Security Remove Device (ZC) 
Objective:  DUT as ZC sends APS Remove device and ZDO Mgmt_Leave_req.

        DUT ZC
          |
        gZR1
       |    |
    gZED1 gZED2

gZC:
PANId=0x1AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

gZR1:
Router extended address:
0x 00 00 00 01 00 00 00 00

gZED1:
End Device extended address:
0x 00 00 00 00 00 00 00 01
(sleepy device)

gZED2:
End Device extended address:
0x 00 00 00 00 00 00 00 02 (sleepy device)

Initial Conditions:
1. All devices have joined using the well-known Trust Center Link Key (ZigBeeAlliance09), have performed a Request Key to obtain a new Link Key, and have confirmed that key with the Verify Key Command.  
2. gZED1 and gZED2 both specify an End Device Timeout of 4 minutes.  They have switched off polling at the start of the test.  The test must be run within the 4-minute timeout.
3. The value of the NWK key used initially may be any value, and is known as KEY0 for this test.  The Key Sequence number shall be 0.

Test Procedure:
1. gZED1 shall change its poll rate to once every 3 seconds.
2. DUT ZC shall unicast an APS Remove Device to gZR1 with a Target Address field containing the EUI64 of gZED1.  
3. gZED1 leaves the network
4. gZR1 notifies the DUT ZC that gZED1 left the network.
5. DUT ZC sends a ZDO Mgmt_Leave_req to gZR1 for device gZED2.  
6. gZR1 sends a ZDO Mgmt_Leave_rsp to DUT ZC.
7. gZED2 is not notified about the leave.
8. DUT ZC sends a mgnt_lqi_req  to gZR1.
9. DUT ZC sends a ZDO Mgmt_Leave_req to gZR1 for device gZR1.  
10. gZR1 sends a ZDO Mgmt_Leave_rsp to DUT ZC.
11. gZR1 leaves the network

Pass verdict:
1. The gZED1 immediately starts issuing MAC Data Poll Requests to gZR1 every 3 seconds.
2. gZR1 unicasts a NWK Leave Command to gZED1 with the Request bit set to TRUE, and Rejoin bit set to FALSE
3. gZED1 sends a NWK Leave Command to gZR1 with Request bit set to FALSE, and Rejoin bit set to FALSE.   gZED1 stops polling.
4. gZR1 sends an APS Update Device command with the following fields: Device (Extended) Address set to the EUI64 address of gZED1, the Short Address field set to the address of gZED1, and Status set to Device Left (0x02).
5. DUT ZC unicasts a ZDO Mgmt_Leave_req with device address set to gZED2’s extended address, Remove Children set to FALSE, and Rejoin set to FALSE.
6. gZR1 unicasts the ZDO Mgmt_Leave_rsp to DUT ZC with the status field set to SUCCESS.
7. gZED2 does not perform a MAC Data Poll and does not receive any packets from gZR1.
8. gZR1send response with mgnt_lqi_rsp without gZED1 and ZED2 entries.
9. DUT ZC unicasts a ZDO Mgmt_Leave_req with device address set to gZR1’s extended address, Remove Children set to FALSE, and Rejoin set to FALSE.
10. gZR1 unicasts the ZDO Mgmt_Leave_rsp to DUT ZC with the status field set to SUCCESS.
11. gZR1 broadcasts a NWK Leave Command with a Radius of 1.  The Command options shall be set as follows:  Rejoin shall be set to FALSE, Request shall be set to FALSE.


