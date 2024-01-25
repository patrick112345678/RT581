TP/PED-4 Child Table Management: Timeout agreement – ZED
Objective: DUT ZED sends End Device Timeout Request to gZC and gZR after joining
This test requires that gZR use the MAC Data poll keepalive method. 
TP/PED-4 and TP/PED-5 are paired test cases that test two methods of keep alive: MAC Data poll keepalive and End Device timeout keepalive. Both test cases are from  point of view of the DUT being the ZED. 


Initial Conditions:

DUT - ZR/ZC

      gZC 
  |       |
  |       |
DUT_ZED   gZR


Test Procedure:
1.	Join device gZR to gZC
2.	Join device DUT ZED  to  gZR
3.	Turn gZR off. Observe the traffic waiting until  DUT ZED emits beacon requests and attempts to rejoin to gZC

Pass Verdict:
1.	DUT ZED shall issue an MLME Beacon Request MAC command frame, and gZR shall respond with a beacon.
2.	DUT ZED is able to complete the MAC association sequence with gZR and gets a new short address, randomly generated.
3.	gZC transports current Network key to DUT ZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	DUT ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	DUT ZED sends an End Device Timeout Request to gZR; with “Requested Timeout Enum” as any value between 0-14; and “End Device Configuration” as 0x00.
6.	gZR sends an End Device Timeout Response to DUT ZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
7.	DUT ZED sends a MAC-Data Poll to gZR, at least three times in every End Device Timeout duration.
8.	gZR sends MAC Acknowledgement for MAC-Data Poll, with Frame Pending” bit as FALSE to DUT ZED.
9.	DUT ZED detects a parent link failure and attempts to rejoin the network

Fail Verdict:
1.	DUT ZED does not issue an MLME Beacon Request MAC command frame, or DUT ZED  does not respond with a beacon.
2.	DUT ZED is not able to complete the MAC association sequence with gZR.
3.	gZC does not send an Transport-Key (NWK key) command to DUT ZED, or gZR does not encrypt the Transport-Key (NWK key) at APS level.
4.	DUT ZED does not issue a device announcement.
5.	DUT ZED does not send an End Device Timeout Request to gZR; or the “Requested Timeout Enum” is not in the range of 0-14; or  “End Device Configuration” bit is not 0x00.
6.	gZR does not send an End Device Timeout Response to DUT ZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
7.	DUT ZED does not send a MAC-Data Poll to  gZR, at least three times in every End Device Timeout duration.
8.	gZR does not send MAC Acknowledgement for MAC-Data Poll to DUT ZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and gZC sends Leave request to DUT ZED immediately after the acknowledgement.
9.	DUT ZED leaves the network or attempts to join another network.
