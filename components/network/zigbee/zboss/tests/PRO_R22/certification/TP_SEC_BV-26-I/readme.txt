11.26	TP/SEC/BV-26-I Security NWK Key Switch (ZR Broadcast)
Objective:  DUT as ZR receives a new NWK Key via broadcast and switches.


DUT ZC	PANId=0x1AAA
	0x0000
	Coordinator external address:0x aa aa aa aa aa aa aa aa

   gZC       
    |          
 DUT ZR1          
    |       
 gZED1           

gZC 
PANId=0x1AAA
0x0000
Coordinator external address:
0x aa aa aa aa aa aa aa aa

DUT ZR1 
Router extended address:	0x 00 00 00 01 00 00 00 00

gZED1	
End Device extended address:	0x 00 00 00 00 00 00 00 01

Initial conditions:
1. All devices have joined using the well-known Trust Center Link Key (ZigBeeAlliance09), have performed a Request Key to obtain a new Link Key, and have confirmed that key with the Verify Key Command. ZC set with permit join enabled and  ZR1 with permit join disabled.  
2. gZED1 specifies an End Device Timeout of 4 minutes.  It has switched off polling at the start of the test.  The test must be run within the  4-minute timeout.
3. The value of the NWK key used initially may be any value, and is known as KEY0 for this test.  The Key Sequence number shall be 0.


Test procedure:
1. gZC selects a new network key via platform specific means.  The key must have a different value than KEY0.  The new key is known as KEY1.  It initiates a broadcast NWK Key update.  
3.gZC shall notify all devices that they must switch to the new network key. 
6. gZC sends a message buffer test request to router DUT ZR1
9. gZED1 initiates a Trust Center Rejoin Request.
10. gZC accepts gZED1 rejoining.
11. gZED1 completes the rejoin.


Pass verdict:
1. gZC Broadcasts an APS Transport Key to the all devices address 0xFFFF.  The message shall only be NWK encrypted with KEY0.  The APS Transport Key contents shall contain KEY1 and the Key sequence shall be 1.
All routers relay the broadcast.  The End Device does not get the APS Transport Key command.  
3. gZC sends an APS Switch Key Command broadcast to the all devices address 0xFFFF. The command shall be encrypted with the new key sequence number 1.  
All routers relay the broadcast.  The End Device does not get the APS switch key command.
6. DUT ZR1 sends back a message buffer test response that is encrypted at the NWK layer with KEY1.
9. DUT ZR1 sends back a Rejoin Response with status of success.   gZED1 sends an APS Update Device command to DUT ZR1.  
10. gZC sends an APS Tunnel command to gZR1.  gZR1 sends an APS Transport Key encrypted with gZED1’s Trust Center link key.  The Transport Key contains KEY1 with key sequence number 1.
11. gZED1 broadcasts a device announce encrypted at the NWK layer with KEY1. 

