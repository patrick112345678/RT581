TP/PED-7 Child Table Management: Timeout agreement – ZC, legacy ZED
Objective: DUT ZC applies default timeout to gZED; the default timeout shall be 10 seconds for this test to speed up the testing. (CCB 2201)

Initial Conditions:

    DUT ZC
   |      |
   |      |
 gZED1  gZED2


Test Procedure:
1. Join device gZED1 to dutZC.
2. Join device gZED2 to dutZC.
3. DUT ZC applies default timeout to gZED; the default timeout shall be 10 seconds for this test to speed up the testing. (CCB 2201)

Pass Verdict:
1.	gZED1 shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
2.	gZED1 is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
3.	DUT ZC transports current Network key to gZED1 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	gZED1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	DUT ZC keeps the Device Timeout as 10 Seconds (default Timeout) for gZED1.
6.	gZED1 polls DUT ZC once in every 5 seconds
7.	DUT ZC sends MAC ACK to gZED1 with Frame Pending bit as FALSE.
8.	gZED2 shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
9.	gZED2 is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
10.	DUT ZC transports current Network key to gZED2 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
11.	gZED2 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
12.	DUT ZC keeps the Device Timeout as 10 Seconds (default Timeout) for gZED2.
13.	DUT ZC shall send a Mgmt_Lqi_rsp to gZED1 and it shall contain only 1 entry for gZED1.

Fail Verdict:
1.	gZED1 does not issue an MLME Beacon Request MAC command frame, or DUT ZC does not respond with a beacon.
2.	gZED1 is not able to complete the MAC association sequence with DUT ZC.
3.	DUT ZC does not send an Transport-Key (NWK key) command to gZED1, or DUT ZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	gZED1 does not issue a device announcement.
5.	DUT ZC does not keep the Device Timeout as 10 Seconds (default Timeout) for gZED1.
6.	gZED1 does not poll DUT ZC once in every 5 seconds
7.	DUT ZC does not send MAC ACK to gZED1; DUT ZC sends MAC ACK to gZED1 with Frame Pending bit as TRUE and send Leave Request to gZED1 immediately after MAC ACK.
8.	gZED2 does not issue an MLME Beacon Request MAC command frame, or DUT ZC does not respond with a beacon.
9.	gZED2 is not able to complete the MAC association sequence with DUT ZC.
10.	DUT ZC does not send an Transport-Key (NWK key) command to gZED2, or DUT ZC does not encrypt the Transport-Key (NWK key) at APS level.
11.	gZED2 does not issue a device announcement.
12.	DUT ZC does not keep the Device Timeout as 10 Seconds (default Timeout) for gZED2.
13.	DUT ZC does not send a Mgmt_Lqi_rsp, or the Mgmt_Lqi_rsp does not contain only 1 entry for gZED1.
