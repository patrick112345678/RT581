TP/PED-10 Child Table Management: Parent Announcement – Previous Parent
Objective: After power cycle, node can confirm whether it is still 
parent for its End Devices by using Parent_annce. 
gZED joins with dutZR and establish Device Timeout with dutZR. 
Later gZED joins with gZC and establish Device Timeout with gZC

        gZC
         |
      DUT ZR
         |
       gZED

gZC	
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = 0x0000
Long Address = 
0x AAAA AAAA AAAA AAAA

DUT ZR	
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address = 
0x 0000 0001 0000 0000

gZED	
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = Generated in a random manner within the range 1 to 0xFFF7
Long Address = 
0x 0000 0000 0000 0001

Test procedure:
1.	Join node DUT ZR with gZC
2.	Join node gZED with DUT ZR
3.	Turn-off DUT ZR for some time. 
4.	Turn-on DUT ZR
5.	gZC send Buffer Test Request to gZED

Pass verdict:
1.	DUT ZR shall issue an MLME Beacon Request MAC command frame, and gZC shall respond with a beacon.
2.	DUT ZR is able to complete the MAC association sequence with gZC and gets a new short address, randomly generated.
3.	gZC transports current Network key to DUT ZR using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	DUT ZR issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	gZED shall issue an MLME Beacon Request MAC command frame, and DUT ZR shall respond with a beacon.
6.	gZED is able to complete the MAC association sequence with DUT ZR and gets a new short address, randomly generated.
7.	gZC transports current Network key to gZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
8.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
9.	gZED sends an End Device Timeout Request to dutZR; with “Requested Timeout Enum” as 1; and “End Device Configuration” as 0x00.
10.	DUT ZR sends an End Device Timeout Response to gZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
11.	gZED sends a MAC-Data Poll to dutZR, atleast once in every End Device Timeout duration.
12.	DUT ZR sends MAC Acknowledgement for MAC-Data Poll with “Frame Pending” bit  as FALSE to gZED.
13.	gZED’s MAC.Data Poll and/or End Device Timeout Request fails continously
14.	gZED rejoins with gZC
15.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
16.	gZED sends an End Device Timeout Request to gZC; with “Requested Timeout Enum” as 0; and “End Device Configuration” as 0x00.
17.	gZC sends an End Device Timeout Response to gZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
18.	gZED sends a MAC-Data Poll to gZC, at least once every End Device Timeout duration.
19.	gZC sends MAC Acknowledgement for MAC-Data Poll, with “Frame Pending” bit as FALSE to gZED.
20.	DUT ZR sends Parent_annce with gZED’s detail.
21.	gZC sends Parent_annce_Resp with gZED’s detail, to dutZR.
22.	DUT ZR deletes the gZED info from Neighbor Table. This is verified by gZC querying DUT ZR’s neighbour table by sending as many mgmt_lqi_req commands as necessary to reveal dutZR’s complete neighbour table.
23.	DUT ZR replies for Route Request  on behalf of gZED; and DUT ZR forwards the Buffer Test Request to gZED
24.	gZED sends Buffer Test Response to DUT ZR.


Fail verdict:
1.	DUT ZR does not issue an MLME Beacon Request MAC command frame, or gZC does not respond with a beacon.
2.	DUT ZR is not able to complete the MAC association sequence with gZC.
3.	gZC does not send an Transport-Key (NWK key) command to DUT ZR, or gZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	DUT ZR does not issue a device announcement.
5.	gZED does not issue an MLME Beacon Request MAC command frame, or DUT ZR does not respond with a beacon.
6.	gZED is not able to complete the MAC association sequence with DUT ZR.
7.	gZC does not send an Transport-Key (NWK key) command to gZED, or gZC does not encrypt the Transport-Key (NWK key) at APS level.
8.	gZED does not issue a device announcement.
9.	gZED does not send an End Device Timeout Request to dutZR; or the “Requested Timeout Enum” is not 1; or  “End Device Configuration” is not 0x00.
10.	DUT ZR does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
11.	gZED does not send a MAC-Data Poll to dutZR, atleast once in every End Device Timeout duration.
12.	DUT ZR does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending “ bit in MAC Acknowledgement is TRUE and dutZR sends Leave request to gZED immediately after the acknowledgement.
13.	DUT ZR sends MAC ACK for gZED’s MAC.Data Poll and/or End Device Timeout Request
14.	gZED does not rejoin with gZC 
15.	gZED does not issue a device announcement.
16.	gZED does not send an End Device Timeout Request to gZC; or the “Requested Timeout Enum” is not 0; or  “End Device Configuration” is not 0x00.
17.	gZC does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
18.	gZED does not send a MAC-Data Poll to gZC, at least once every End Device Timeout duration.
19.	gZC does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and gZC sends Leave request to gZED immediately after the acknowledgement.
20.	DUT ZR does not send Parent_annce with gZED details.
21.	gZC does not send Parent_annce_Resp with gZED’s detail, to dutZR.
22.	DUT ZR does not delete gZED info from Neighbor Table.
23.	DUT ZR does not reply for Route Request  on behalf of gZED; DUT ZR does not forward the Buffer Test Request to gZED; or dutZR replies for Route Request  on behalf of gZED
24.	gZED does not send Buffer Test Response to DUT ZR.
