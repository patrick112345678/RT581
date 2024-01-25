TP/PED-12 Child Table Management: Parent Announcement – Current Parent
Objective: After power cycle, node can confirm whether it is still parent for its End Devices by using Parent_annce. gZED joins with gZR1 and establish Device Timeout with gZR1. Later gZED joins with DUT ZC and establishes Device Timeout with DUT ZC. 

        DUT ZC
          |      
        gZR1     
          |
        gZED

DUT ZC	
EPID = 0x0000 0000 0000 0001
PAN ID = 0x1aaa
Short Address = 0x0000
Long Address = 
0x AAAA AAAA AAAA AAAA

gZR1	
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
1.	Join node gZR1 with DUT ZC
2.	Join node gZED with gZR1
3.	Turn-off gZR1 for some time. 
4.	Turn-on gZR1
5.	DUT ZC send Buffer Test Request to gZED

Pass verdict:
1.	gZR1 shall issue an MLME Beacon Request MAC command frame, and DUT ZC shall respond with a beacon.
2.	gZR1 is able to complete the MAC association sequence with DUT ZC and gets a new short address, randomly generated.
3.	DUT ZC transports current Network key to gZR1 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	gZR1 issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	gZED shall issue an MLME Beacon Request MAC command frame, and gZR1 shall respond with a beacon.
6.	gZED is able to complete the MAC association sequence with gZR1 and gets a new short address, randomly generated.
7.	DUT ZC transports current Network key to gZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
8.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
9.	gZED sends an End Device Timeout Request to gZR1; with “Requested Timeout Enum” as 1; and “End Device Configuration” as 0x00.
10.	gZR1 sends an End Device Timeout Response to gZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
11.	gZED sends a MAC-Data Poll to gZR1, atleast once in every End Device Timeout duration.
12.	gZR1 sends MAC Acknowledgement for MAC-Data Poll, with Frame Pending” bit as FALSE to gZED.
13.	gZED’s MAC.Data Poll fails continously
14.	gZED rejoins with DUT ZC
15.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
16.	gZED sends an End Device Timeout Request to DUT ZC; with “Requested Timeout Enum” as 0; and “End Device Configuration” as 0x00.
17.	DUT ZC sends an End Device Timeout Response to gZED, with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported” as TRUE.
18.	gZED sends a MAC-Data Poll to DUT ZC, atleast once in every End Device Timeout duration.
19.	DUT ZC sends MAC Acknowledgement for MAC-Data Poll, with Frame Pending” bit as FALSE to gZED.
20.	gZR1 sends Parent_annce with gZED’s age detail.
21.	DUT ZC sends Parent_annce_Resp with gZED’s age detail, to gZR1.
22.	gZR1 compares the gZED’s local age and received age. And deletes the gZED info from Neighbor Table.
23.	DUT ZC replies for Route Request  on behalf of gZED; and DUT ZC forwards the Buffer Test Request to gZED
24.	gZED sends Buffer Test Response to DUT ZC.

Fail verdict:
1.	gZR1 does not issue an MLME Beacon Request MAC command frame, or DUT ZC does not respond with a beacon.
2.	gZR1 is not able to complete the MAC association sequence with DUT ZC.
3.	DUT ZC does not send an Transport-Key (NWK key) command to gZR1, or DUT ZC does not encrypt the Transport-Key (NWK key) at APS level.
4.	gZR1 does not issue a device announcement.
5.	gZED does not issue an MLME Beacon Request MAC command frame, or gZR1 does not respond with a beacon.
6.	gZED is not able to complete the MAC association sequence with gZR1.
7.	DUT ZC does not send an Transport-Key (NWK key) command to gZED, or DUT ZC does not encrypt the Transport-Key (NWK key) at APS level.
8.	gZED does not issue a device announcement.
9.	gZED does not send an End Device Timeout Request to gZR1; or the “Requested Timeout Enum” is not 1; or  “End Device Configuration” is not 0x00.
10.	gZR1 does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
11.	gZED does not send a MAC-Data Poll to gZR1, atleast once in every End Device Timeout duration.
12.	gZR1 does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and gZR1 sends Leave request to gZED immediately after the acknowledgement.
13.	gZR1 sends MAC ACK for gZED’s MAC.Data Poll
14.	gZED does not rejoin with DUT ZC 
15.	gZED does not issue a device announcement.
16.	gZED does not send an End Device Timeout Request to DUT ZC; or the “Requested Timeout Enum” is not 0; or  “End Device Configuration” is not 0x00.
17.	DUT ZC does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” as FALSE.
18.	gZED does not send a MAC-Data Poll to DUT ZC, atleast once in every End Device Timeout duration.
19.	DUT ZC does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT ZC sends Leave request to gZED immediately after the acknowledgement.
20.	gZR1 does not send Parent_annce with gZED details.
21.	DUT ZC does not send Parent_annce_Resp with gZED’s age detail, to gZR1.
22.	gZR1 does not delete gZED info from Neighbor Table.
23.	DUT ZC does not reply for Route Request  on behalf of gZED; DUT ZC does not forward the Buffer Test Request to gZED; or gZR1 replies for Route Request  on behalf of gZED
24.	gZED does not send Buffer Test Response to DUT ZC.
