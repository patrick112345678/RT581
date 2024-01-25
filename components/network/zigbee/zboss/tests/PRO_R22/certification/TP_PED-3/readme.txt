TP/PED-3 Child Table Management: End Device Initiator – ZR/ZC
Objective: DUT ZR/ZC clears end-device initiator bit in NWK header for all forwarded frames. Test case used to validate DUT ZR/ZC correctly handles end devices using end device keepalive with 2 minute timeout and DUT ZC/ZR 

Initial Conditions:

DUT - ZR/ZC

    DUT
  |     |
  |     |
gZED1 gZED2


Test Procedure:
1.	Let DUT form a network, either centralized (if DUT is a ZC) or distributed (if DUT is a ZR)
2.	Join device gZED1 to DUT.
3.	Join device gZED2 to DUT.
4.	Let DUT send Buffer Test Request to gZED1
5.	Let gZED2 sends Buffer Test Request to gZED1
6.	Factory reset gZED1 and keep it off for two minutes, such that DUT ages gZED1 out of its neighbour table.
7.	Let DUT open the network for joining
8.	By implementation specific means the DUT ZC shall set AllowJoins policy to TRUE. (CCB 2133)
9.	Let gZED1 associate from scratch, e.g. by factory resetting gZED1

Pass Verdict:
1.	After MAC association, DUT transports current Network key to gZED1 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
2.	DUT sends an End Device Timeout Response after gZED1 sent a End-Device Timeout request with timeout = 1 (2 minutes) and configuration = 0. The Timeout response reports “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive or End Device Timeout Keep alive Supported” as TRUE (CCB 2144).
3.	If polled by gZED1, DUT sends MAC ACK for MAC-Data Poll, with “Frame Pending” bit as FALSE to gZED1 (or “Frame Pending” bit set to true, followed by an empty MAC data frame) (CCB 2203) .(CCB  2244)
4.	After MAC association, DUT transports current Network key to gZED2 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level.
5.	DUT sends an End Device Timeout Response after gZED2 sent a End-Device Timeout request with timeout = 1 (2 minutes) and configuration = 0. The Timeout response reports “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive or End Device Timeout Keep alive Supported” as TRUE. (CCB 2144)
6.	If polled by gZED2, DUT sends MAC ACK for MAC-Data Poll, with “Frame Pending” bit as FALSE to gZED2 (or “Frame Pending” bit set to true, followed by an empty MAC data frame). (CCB 2244)
7.	DUT sends Buffer Test Request to gZED1 with End Device Initiator bit set as FALSE, which is answered by a buffer test response
8.	When gZED2 sends a Buffer Test Request to gZED1 via DUT, DUT forwards the Buffer Test Request to gZED1 and clears the End Device Initiator bit when forwarding
9.	When gZED1 sends Buffer Test Response to gZED2 via DUT, DUT forwards the Buffer Test Respone to gZED2 and clears the End Device Initiator bit when forwarding
10.	After MAC association, DUT transports current Network key to gZED1 using APS Transport-Key; APS Transport-Key encrypted with shared link key at APS level. DUT does not send a leave message.
11.	DUT sends an End Device Timeout Response after gZED1 sent a End-Device Timeout request with timeout = 1 (2 minutes) and configuration = 0. The Timeout response reports “Status” as SUCCESS and “Parent information - MAC Data Poll Keep alive or End Device Timeout Keep alive Supported” as TRUE. (CCB 2144)
12.	If polled by gZED1, DUT sends MAC ACK for MAC-Data Poll, with “Frame Pending” bit as FALSE to gZED1 (or “Frame Pending” bit set to true, followed by an empty MAC data frame).(CCB 2244)

Fail Verdict:
1.	DUT does not send an Transport-Key (NWK key) command to gZED1, or dutZC does not encrypt the Transport-Key (NWK key) at APS level.
2.	DUT does not send an End Device Timeout Response to gZED1; or the “Status” is not SUCCESS; or both the “Parent information - MAC Data Poll Keep alive Supported” and the “Parent information – and End Device Timeout  Keep alive Supported” are reported as FALSE.
3.	DUT does not send MAC Acknowledgement for MAC-Data Poll to gZED1; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT sends Leave request to gZED1 immediately after the MAC ACK.
4.	DUT does not send an Transport-Key (NWK key) command to gZED2, or DUT does not encrypt the Transport-Key (NWK key) at APS level.
5.	DUT does not send an End Device Timeout Response to gZED2; or the “Status” is not SUCCESS; or both the “Parent information - MAC Data Poll Keep alive Supported” and the “Parent information – End Device Timeout Keep alive Supported” are reported as FALSE.
6.	DUT does not send MAC Acknowledgement for MAC-Data Poll to gZED2; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT sends Leave request to gZED2 immediately after the MAC ACK.
7.	DUT does not send Buffer Test Request to gZED1; or the End Device Initiator bit set as TRUE in Buffer Test Request; or the gZED does not answer the buffer test request due to the request sent by the DUT being malformed
8.	DUT does not forward the buffer test request; or the End Device initiator bit is set in the forwarded frame
9.	gZED1 does not send Buffer Test Response to gZED2; or the End Device Initiator bit set as FALSE in Buffer Test Response
10.	DUT does not send an Transport-Key (NWK key) command to gZED1, or dutZC does not encrypt the Transport-Key (NWK key) at APS level.
11.	DUT does not send an End Device Timeout Response to gZED1; or the “Status” is not SUCCESS; or both the “Parent information - MAC Data Poll Keep alive Supported” and the “Parent information – End Device Timeout Keep alive Supported” are reported as FALSE.
12.	DUT does not send MAC Acknowledgement for MAC-Data Poll to gZED1; or “Frame Pending” bit in MAC Acknowledgement is TRUE and DUT sends Leave request to gZED1 immediately after the MAC ACK.

Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc or zr: i.e. ./runng zc to start test with DUT as ZC.
