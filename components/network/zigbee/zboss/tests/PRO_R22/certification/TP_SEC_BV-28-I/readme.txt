11.28	TP/SEC/BV-28-I Security Remove Device (ZR)
Objective:  DUT as ZR correctly handles APS Remove device and ZDO Mgmt_Leave_req


         gZC
          |
        DUT ZR1
          |
        gZR2
       |      |
     gZED1  gZED2
   
gZC:
PANId=0x1AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1:
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
1. All devices have joined using the well-known Trust Center Link Key (ZigBeeAlliance09), have performed a Request Key to obtain a new Link Key, and have confirmed that key with the Verify Key Command. ZC set with permit join enabled and  ZR1 with permit join disabled.
2. gZED1 and gZED2 both specify an End Device Timeout of 4 minutes.  It has switched off polling at the start of the test.  The test must be run within the 4-minute timeout.
3. The value of the NWK key used initially may be any value, and is known as KEY0 for this test.  The Key Sequence number shall be 0.

Test Procedure:
1. gZED1 shall change its poll rate to once every 3 seconds.
2. gZC shall unicast an APS Remove Device to DUT ZR1 with a Target Address field containing the EUI64 of gZED1.  
3. gZED1 leaves the network
4. DUT ZR1 notifies the gZC that gZED1 left the network.
5. gZC sends a ZDO Mgmt_Leave_req to DUT ZR1 for a non-existent device with address 0xDDDDDDDDDDDDDDDD.
6. DUT ZR1 sends a ZDO Mgmt_Leave_rsp to gZC.
7. gZC sends a ZDO Mgmt_Leave_req to DUT ZR1 for device gZED2.  
8. DUT ZR1 sends a ZDO Mgmt_Leave_rsp to gZC.
9. gZED2 is not notified about the leave.
10. gZC sends  a mgnt_lqi_req  to DUT ZR1.
11. gZC sends a ZDO Mgmt_Leave_req to DUT ZR1 for device DUT ZR1.  
12. DUT ZR1 sends a ZDO Mgmt_Leave_rsp to gZC.
13. DUT ZR1 leaves the network


Pass verdict:
1. The gZED1 immediately starts issuing MAC Data Poll Requests to DUT ZR1 every 3 seconds.
2. DUT ZR1 unicasts a NWK Leave Command to gZED1 with the Request bit set to TRUE, and Rejoin bit set to FALSE
3. gZED1 sends a NWK Leave Command to DUT ZR1 with Request bit set to FALSE, and Rejoin bit set to FALSE.   gZED1 stops polling.
4. DUT ZR1 sends an APS Update Device command with the following fields: Device (Extended) Address set to the EUI64 address of gZED1, the Short Address field set to the address of gZED1, and Status set to Device Left (0x02).
5. gZC unicasts a ZDO Mgmt_Leave_req with Device (Extended) Address set to 0xDDDDDDDDDDDDDDDD, Remove Children set to FALSE, and Rejoin set FALSE.  
6. DUT ZR1 unicasts the ZDO Mgmt_Leave_rsp to gZC with the status field set to UNKNOWN_DEVICE.  
7. gZC unicasts a ZDO Mgmt_Leave_req with device address set to gZED2’s extended address, Remove Children set to FALSE, and Rejoin set to FALSE.
8. DUT ZR1 unicasts the ZDO Mgmt_Leave_rsp to gZC with the status field set to SUCCESS.
9. gZED2 does not perform a MAC Data Poll and does not receive any packets from DUT ZR1.
10. DUT ZR1send response with mgnt_lqi_rsp without gZED1 and ZED2 entries.
11. gZC unicasts a ZDO Mgmt_Leave_req with device address set to DUT ZR1’s extended address, Remove Children set to FALSE, and Rejoin set to FALSE.
12. DUT ZR1 unicasts the ZDO Mgmt_Leave_rsp to gZC with the status field set to SUCCESS.
13. DUT ZR1 broadcasts a NWK Leave Command with a Radius of 1.  The Command options shall be set as follows:  Rejoin shall be set to FALSE, Request shall be set to FALSE.

