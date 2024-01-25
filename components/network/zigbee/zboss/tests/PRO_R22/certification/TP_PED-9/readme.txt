TP/PED-9 Child Table Management: Child device Leave and Rejoin when age out by Parent
Objective:  DUT ZED shall send End Device Timeout Request to gZC after successfully join in the network. gZC shall age out the End Device if the End Device doesn’t send Keep Alive within End Device Timeout

Initial Conditions:

   gZC
    |
    |
    |
  DUT ZED


Test Procedure:
1. Join device DUT ZED to gZC.
2. Change the “Device Timeout” in Neighbor Table as 10 seconds in gZC by implementation specific way.


Pass Verdict:
1.	DUT ZED shall issue an MLME Beacon Request MAC command frame, and ZC shall respond with a beacon.
2.	DUT ZED is able to complete the MAC association sequence with ZC and gets a new short address, randomly generated.
3.	gZC transports current Network key to DUT ZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	DUT ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	DUT ZED sends an End Device Timeout Request to ZC; with “Requested Timeout Enum” as 1 (2 minutes); and “End Device Configuration” as 0x00.
6.	gZC sends an End Device Timeout Response to DUT ZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
7.	DUT ZED sends a MAC-Data Poll to ZC, at least once in every 2 minutes.
8.	gZC sends MAC Acknowledgement for MAC-Data Poll, with Frame Pending” bit as FALSE to DUT ZED.
9.	gZC waits until new Device Timeout (10 seconds) ages out the DUT ZED. When DUT ZED sends MAC.Data Poll Keep Alive next time, ZC sends the MAC-ACK with “Frame Pending” as TRUE followed by Leave Request to DUT ZED.
10.	DUT ZED performs a Rejoin with gZC and initiates End Device Timeout

Fail Verdict:
1.	DUT ZED does not issue an MLME Beacon Request MAC command frame, or ZC does not respond with a beacon.
2.	DUT ZED is not able to complete the MAC association sequence with ZC.
3.	gZC does not send an Transport-Key (NWK key) command to DUT ZED, or ZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	DUT ZED does not issue a device announcement.
5.	DUT ZED does not send an End Device Timeout Request to ZC; or the “Requested Timeout Enum” is not 1; or  “End Device Configuration” is not 0x00.
6.	gZC does not send an End Device Timeout Response to DUT ZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
7.	DUT ZED does not send a MAC-Data Poll to ZC, at least once in every 2 minutes.
8.	gZC does not send MAC Acknowledgement for MAC-Data Poll to DUT ZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and gZC sends Leave request to DUT ZED immediately after the acknowledgement.
9.	gZC does not age out the DUT ZED as per new Device Timeout. DUT ZED does not send MAC.Data Poll Keep Alive as per its Device Timeout.When DUT ZED sends MAC.Data Poll Keep Alive next time, ZC does not send the MAC-ACK with “Frame Pending” as TRUE followed by Leave Request to DUT ZED.
10.	DUT ZED does not Rejoin with gZC; DUT ZED does not initiate End Device Timeout with ZC
