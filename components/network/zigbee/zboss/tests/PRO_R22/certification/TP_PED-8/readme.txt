TP/PED-8 Child Table Management: Make ZED leave by injecting NWK leave.
Objective:  DUT ZR/ZC injects NWK leave into MAC indirect queue in response to unknown child polling subject to applicable timing constraints (CCB 2248).

Initial Conditions:
     DUT ZC
        |
      gZED
      
Test Procedure:
1.	Join device gZED to DUT ZC.
2.	Change the poll rate of gZED to two minutes by implementation specific means

Pass Verdict:
1.	gZED shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
2.	gZED is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
3.	DUT ZC transports current Network key to gZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	gZED sends an End Device Timeout Request to DUT ZC; with “Requested Timeout Enum” as 0 (10 seconds); and “End Device Configuration” as 0x00.
6.	DUT ZC sends an End Device Timeout Response to gZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
7.	gZED sends a MAC-Data Poll to DUT ZC, atleast once in every 10 seconds
8.	DUT ZC sends MAC Acknowledgement for MAC-Data Poll, with Frame Pending” bit as FALSE to gZED.
9.	DUT ZC waits till Device Timeout duration and aging out gZED. When gZED sends MAC.Data Poll Keep Alive next time, DUT ZC sends the MAC-ACK with “Frame Pending” as TRUE followed by Leave Request to gZED.
10.	gZED does Rejoin with DUT ZC and initiates End Device Timeout

Fail Verdict:
1.	gZED does not issue an MLME Beacon Request MAC command frame, or DUT ZC does not respond with a beacon.
2.	gZED is not able to complete the MAC association sequence with DUT ZC.
3.	DUT ZC does not send an Transport-Key (NWK key) command to gZED, or DUT ZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	gZED does not issue a device announcement.
5.	gZED does not send an End Device Timeout Request to DUT ZC; or the “Requested Timeout Enum” is not 0; or  “End Device Configuration” is not 0x00.
6.	DUT ZC does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
7.	DUT ZED does not send a MAC-Data Poll to ZC, atleast once in every 10 seconds
8.	DUT ZC does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT ZC sends Leave request to gZED immediately after the acknowledgement.
9.	DUT ZC does not age out the gZED as per new Device Timeout. When gZED sends MAC.Data Poll Keep Alive next time,  DUT ZC does not send the MAC-ACK with “Frame Pending” as TRUE followed by Leave Request to DUT ZED.
10.	gZED does not Rejoin with DUT ZC; gZED does not initiate End Device Timeout with DUT ZC
