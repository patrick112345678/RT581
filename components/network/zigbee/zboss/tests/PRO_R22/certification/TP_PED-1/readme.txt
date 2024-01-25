TP/PED-1 Child Table Management: Changing parents (online)
Objective: Make sure that DUT ZC and gZR maintains its child table when one of its child devices, gZED, changes parents while DUT ZC is online.

Initial Conditions:

   DUT ZC
  |     |
  |     |
gZED   gZR


Test Procedure:
1.	Make sure that DUT ZC, gZR, and gZED are on the same network and able to communicate, i.e.
•	DUT is a ZC, join -gZR to DUT ZC; join gZED to DUT ZC.
2.	Power off and silently remove gZED from the network so that it does not send a NWK leave command or any other indication to the DUT ZC that it has been removed.
3.	Before the negotiated end device timeout elapses, spoof the Device_Annce message of gZED from gZR using the ZGP alias addressing feature to make it appear that gZED has changed parents. The Device_Annce message should contain:
•	The MAC source address and NWK extended nonce of the-gZR.
•	The broadcast address of 0xffff in the MAC destination address.
•	The NWK source address and Device_Annce payload of gZED.
•	The broadcast address of 0xfffd in the NWK destination address.
4.	Send repeated Mgmt_lqi_req messages from the gZR with an increasing startIndex field to the DUT until the entire neighbour table has been read from the DUT ZC.

Pass Verdict:
1.	gZR  responds to data polls or end device timeout requests from gZED, indicating an intact parent/child relationship.(CCB 2144)
2.	gZR rebroadcasts the spoofed Device_Annce from gZED
3.	DUTZC responds to each Mgmt_lqi_req with a Mgmt_lqi_rsp. The neighbour table entries returned by the Mgmt_lqi_rsp commands may contain either one or two unique entries. At least one entry must contain the address of gZR. If there is a second entry, it must contain the address of gZED with a relationship of "previous child".

Fail Verdict:
1. gZR does not respond to data polls or end device timeout requests, or issues a leave command to the gZED.
3. DUT ZC responds with a neighbour table entry containing the address of gZED with a relationship other than "previous child".

Additional info:
 - To start test use ./runng.sh <dut_role>, where <dut_role> can be zc or zr: i.e. ./runng zc to start test with DUT as ZC.
