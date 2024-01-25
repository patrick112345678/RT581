TP/PED-5 Child Table Management: Timeout agreement – ZED
Objective: DUT ZED sends End Device Timeout Request to gZC/gZR after joining.

Initial Conditions:

DUT - ZR/ZC

      gZC 
  |       |
  |       |
DUT_ZED   gZR


Test Procedure:
1.	Join device gZR to gZC
2.	Join device DUT ZED to gZR
3.	Turn gZR off. Observe the traffic waiting until DUT ZED emits beacon requests and attempts to rejoin

Pass Verdict:
1.	DUT ZED shall issue an MLME Beacon Request MAC command frame, andgZRshall respond with a beacon.
2.	DUT ZED is able to complete the MAC association sequence with gZR and gets a new short address, randomly generated.
3.	gZC transports current Network key to DUT ZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	DUT ZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	DUT ZED sends an End Device Timeout Request to gZR; with “Requested Timeout Enum” as any value between 0-14; and “End Device Configuration” as 0x00.
6.	gZR sends an End Device Timeout Response to DUT ZED, with “Status” as SUCCESS and “Parent information End Device Timeout Request Keepalive Supported” as TRUE.
7.	DUT ZED sends an End Device Timeout Request to gZR, at least three times in every End Device Timeout duration.
8.	gZR sends End Device Timeout Response
9.	DUT ZED detects a parent link failure and attempts to rejoin the network

Fail Verdict:
1.	DUT ZED does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2.	DUT ZED is not able to complete the MAC association sequence with gZC.
3.	gZC does not send an Transport-Key (NWK key) command to DUT ZED, or gZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	DUT ZED does not issue a device announcement.
5.	DUT ZED does not send an End Device Timeout Request to gZR; or the “Requested Timeout Enum” is not in the range of 0-14; or  “End Device Configuration” bit is not 0x00.
6.	gZR does not send an End Device Timeout Response to DUT ZED; or the “Status” is not SUCCESS; or the “Parent information - End Device Timeout Request Keepalive Supported” as FALSE.
7.	DUT ZED does not send an End Device Timeout Request to gZR, at least three times in every End Device Timeout duration.
8.	gZR does not End Device Timeout Response.
9.	DUT ZED leaves the network or attempts to join another network.
