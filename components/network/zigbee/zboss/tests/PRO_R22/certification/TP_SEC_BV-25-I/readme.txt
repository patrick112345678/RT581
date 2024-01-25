11.25	TP/SEC/BV-25-I Security NWK Key Switch (TC unicasts) 
Objective: DUT ZC unicasts a new NWK Key and triggers a switch.


DUT ZC:
PANId=0x1AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

gZR1:
Router extended address:
0x 00 00 00 01 00 00 00 00

gZR2:
Router extended address:
0x 00 00 00 02 00 00 00 00

gZED1:
End Device extended address:
0x 00 00 00 00 00 00 00 01

Initial Conditions:
1. All devices have joined using the well-known Trust Center Link Key (ZigBeeAlliance09), have performed a Request Key to obtain a new Link Key, and have confirmed that key with the Verify Key Command. ZC set with permit join enabled and  ZR1 with permit join disabled. 
2. gZED1 specifies an End Device Timeout of  4 minutes.  It has switched off polling at the start of the test.  The test must be run within the 4-minute timeout.
3. The value of the NWK key used initially may be any value, and is known as KEY0 for this test.  The Key Sequence number shall be 0.

       DUT ZC
         |
       gZR1
      |    |
    gZR2 gZED1

Test Procedure:

1. DUT ZC selects a new network key via platform specific means.  The key must have a different value than KEY0.  The new key is known as KEY1.  DUT ZC initiates a unicast NWK Key update to gZR1.  
2. DUT ZC shall not send gZR2 a copy of the new Network key.
3. DUT ZC shall notify all devices that they must switch to the new network key. 
5. DUT ZC shall send a message buffer test request.to gZR2.
6. gZR2 does not generate a message buffer testresponse.
7. Switch off gZR2.  This prevents gZED1 from joining the wrong router.
8. gZED1 initiates a Trust Center Rejoin Request.
9. DUT ZC accepts gZED1 rejoining.
10. gZED1 completes the rejoin.


Pass verdict:
1. DUT ZC unicasts the APS Transport Key to gZR1.  The message shall be NWK encrypted with KEY0, and APS Encrypted with gZR1’s Trust Center Link Key.  The APS Transport Key contents shall contain KEY1 and the Key sequence shall be 1.
2. No APS Transport Key command is sent to gZR2.
3. DUT ZC sends an APS Switch Key Command broadcast to the all devices address 0xFFFF. The command shall be encrypted with the new key sequence number 1.  
All routers relay the broadcast.  The End Device does not get the APS switch key command.
5. DUT ZC sends an APS datagram unicast to gZR2.  It shall be encrypted at the NWK layer using KEY1.
7. gZR2 sends no packets.
8. gZR1 sends back a Rejoin Response with status of success.   gZR1 sends an APS Update Device command to DUT ZC.  
9. DUT ZC sends an APS Tunnel command to gZR1.  gZR1 sends an APS Transport Key encrypted with gZED1’s Trust Center link key.  The Transport Key contains KEY1 with key sequence number 1.
10. gZED1 broadcasts a device announce encrypted at the NWK layer with KEY1. 

