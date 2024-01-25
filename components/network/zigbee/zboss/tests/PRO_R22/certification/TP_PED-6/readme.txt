TP/PED-6 Child Table Management: Timeout agreement – ZR/ZC, R21+ ZED
Objective: DUT ZR/ZC sends End Device Timeout Response when gZED (R21+) joins;
           removes child from child table after missing keep-alive

Initial Conditions:

DUT - ZR/ZC

    DUT 
  |     |
  |     |
gZED   gZR

Test Setup:
The DUT and gZR and gZED are in wireless communication proximity. A packet sniffer shall be observing the
communication over the air interface.


Test Procedure:
1.	Join device gZED to DUT
2.	Join device gZR to DUT
3.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 30 seconds after gZED has performed timeout negotiation
4.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 60 seconds after gZED has performed timeout negotiation
5.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 90 seconds after gZED has performed timeout negotiation
6.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 120 seconds after gZED has performed timeout negotiation.
7.	Turn gZED off
8.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 60 seconds after gZED has been turned off
9.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 90 seconds after gZED has beeed turned off
10.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 120 seconds after gZED has been turned off
11.	Let gZR send any number of mgmt_lqi_req required to reveal DUTs complete neighbour table 150 seconds after gZED has been turned off

Pass Verdict:
1.	gZED shall issue an MLME Beacon Request MAC command frame, and DUT  shall respond with a beacon.
2.	gZED is able to complete the MAC association sequence with DUT  and gets a new short address, randomly generated.
3.	DUT  transports current Network key to gZED using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
4.	gZED issues a ZDO device announcement sent to the broadcast address (0xFFFD).
5.	In response to gZED sending an End Device Timeout Request to DUT with “Requested Timeout Enum” as set in the initial conditions above and “End Device Configuration” as 0, DUT sends an End Device Timeout Response to gZED, 
6.	with “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive Supported”  as TRUE or “Parent information – End Device Timeout Keep alive Supported” as TRUE  (CCB 2144)
7.	If  polled by gZED1, DUT sends MAC Acknowledgement for MAC-Data Poll, with “Frame Pending” bit as FALSE to gZED. It is also permissible for “Frame Pending” bit being set to true, followed by an empty MAC data frame (CCB 2246).
8.	In one of the mgmt_lqi_rsp sent by DUT in response to the mgmt_lqi_req sent by gZR, there is a valid an entry for gZED with relation = child
9.	In none of the mgmt_lqi_rsp sent by DUT in response to the mgmt_lqi_req sent by gZR, there is an entry for gZED.

Fail Verdict:
1.	DUT  does not respond with a beacon.
2.	gZED is not able to complete the MAC association sequence with DUT .
3.	DUT  does not send an Transport-Key (NWK key) command to gZED, or ZC  does not encrypt the Transport-Key (NWK key) at APS level.
4.	gZED does not issue a device announcement.
5.	DUT  does not send an End Device Timeout Response to gZED; or the “Status” is not SUCCESS; or the “Parent information - MAC Data Poll Keep alive Supported” and the “Parent information – End Device Timeout Keep alive Supported” are both FALSE.
6.	DUT does not send MAC Acknowledgement for MAC-Data Poll to gZED; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT  sends Leave request to gZED immediately after the acknowledgement.
7.	In none of the mgmt_lqi_rsp sent by DUT in response to the mgmt_lqi_req sent by gZR, there is a valid an entry for gZED; or the entry indicates a relation other than child
8.	In any of the mgmt_lqi_rsp sent by DUT in response to the mgmt_lqi_req sent by gZR, there is an entry for gZED
