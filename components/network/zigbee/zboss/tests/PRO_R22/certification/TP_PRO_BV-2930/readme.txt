TP/PRO/BV-29 PanID conflict detection - ZR
Verify that when the 16-bit PANID is not a unique number there is a PANId Conflict.

Test run instruction:
- run zc1, wait for beacon req
- run zr2, wait for join complete
- run zc2, wait for beacon req
- run zr3, wait for beacon req and beacons
- wait for 75 seconds - data from zr1

{TP/PRO/BV-30 PanID conflict detection - ZC}


gZC1
{gZR1 ???, added, but commented in run.sh}
DUT ZR2
gZR3
gZC2

Graph looks like:
 1 gZC1
 |            {|   }
 2 DUT ZR2    {gZR1}
 |
 3 gZC2
 |
 4 gZR3 (mustn't join)

Test Set-Up:
DUT ZC1, gZR1, and DUT ZR2, are in communication distance.
DUT ZR2, gZC2, and gZC3 are in communication distance.
gZR3, gZC2, and gZC3 are in communication distance.

Initial condition:
1. DUT ZR2 joins gZC1;
2. Set gZC2 under target stack profile, gZC2 as Coordinator starts a PAN with a PANId different from the PANID of DUT ZC1.
3. Assign the PanID of gZC2 to be the same as that of gZC1 by some means.
4. gZC1 SHALL enable automatic PAN ID conflict resolution


Test procedure:
1. Start the gZR3;
2. Test Drive application transmits test buffer request using broadcast, 
to all nodes in the PAN sequence length = 0x10;

Pass verdict:
Expected Outcome
1. gZR3 shall issue a MLME_Beacon Request MAC command frame, and gZC2 shall respond with beacon;
2. Based on the active scan result gZR3 is able to complete the MAC association sequence with gZC2 and gets new short address 
from Coordinator (Generated in a random manner within the range 1 to 0xFFF7);
3. DUT ZR2 is capable of detecting the beacon from gZC2;
4. DUT ZR2 optionally active scans by given beacon request and initiates the network report command to nwkManager (gZC1);
5. DUT ZR2 over the air packet, in the network payload command frame identifier shall be 0x09;
6. DUT ZR2 over the air packet, in the network payload report information count sub-field of the command options  field shall be equal to 02;

NOTE: it is wrong, it must be 1. That description is for old test, which had one more alien zc - see 9.

7. DUT ZR2 over the air packet, in the network payload report Command Identifier  sub-field of the command options field contains the value 0x00;
8. DUT ZR2 over the air packet, in the network payload EPID field shall be 0xaa aa aa aa aa aa aa aa (gZC1 Extended PanID);
9. DUT ZR2 over the air packet, in the network payload report Information field shall be gZC2PanID and gZC3 PanID;
10. Upon network report command unicast message from DUT ZR2, gZC1 initiates network update command to inform about new PAN ID.
11. DUT ZC1 over the air packet, in the MAC frame header destination PanID is set as the old PanID;
12. gZC1 over the air packet, in the MAC and NWK frame header source address are 0x0000;
13. gZC1 over the air packet, in the MAC and NWK frame header destination addres are 0xffff;
14. gZC1 over the air packet, in the network payload command frame identifier shall be 0x0a;
15. gZC1 over the air packet, in the network payload update information
count sub-field of the command options field shall be equal to 01;
16. gZC1 over the air packet, in the network payload update Command Identifier sub-field of the command options field contains the value 0x00;
17. gZC1 over the air packet, in the network payload EPID field shall be xaa aa aa aa aa aa aa aa.
18. gZC1 over the air packet, in the network payload update Information field contains the ZC1 new PanID.
19. gZC1 broadcasts over-the-air APS Data frame to all nodes.
    DstAddrMode=0x02=16-bit
    DstAddr=0xffff=broadcast
    DstEndpoint=0xF0
    ProfileId=0x7f01=Test profileID
    ClusterID=0x001c
    SrcEndpoint=0x01=endpoint 1 on DUT ZC1
    asduLength=length of test MSG Buffer_Test_req
    asdu=Buffer_Test_req with sequence length octet (10)
20. gZC1 over the air packet, in the MAC frame header destination PanID is set as the new PanID.
21. DUT ZR2 shall transmit over-the-air APS Data frame to gZC1.
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=DUT ZC1
    DstEndpoint=0x01=endpoint 1 of DUT ZC1
    ProfileId=0x7f01=Test profileID
    ClusterID=0x0054
    SrcEndpoint=0xF0=endpoint 240 on DUT ZR2
    asduLength=length of test MSG Buffer_Test_rsp
    asdu=Buffer_Test_rsp with sequence length octet (10)
22. DUT ZR2 over the air packet, in the MAC frame header destination PanID is set as the new PanID.

Fail verdict:
1. gZR3 does not issue a MLME_Beacon Request MAC command frame, and gZC2` shall respond with beacon.
2. Based on the active scan result gZR3 is not able to complete the MAC association sequence with gZC2 and does not get new short
address from Coordinator (Generated in a random manner within the range 1 to 0xFFF7)
3. DUT ZR2 is not capable of detecting the beacon from gZC2.
4. DUT ZR2 optionally active scans by given beacon request and does not initiate the network report command to nwkManager (gZC1).
5. DUT ZR2 over the air packet, in the network payload command frame identifier does not 0x09.
6. DUT ZR2 over the air packet, in the network payload report information count sub-field 
of the command options field does not equal to 02
7. DUT ZR2 over the air packet, in the network payload report command Identifier sub-field 
of the command options field does not contain the value 0x00.
8. DUT ZR2 over the air packet, in the network payload EPID field does not 0xaa aa aa aa aa aa aa aa (DUT ZC1 Extended PanID).
9. DUT ZR2 over the air packet, in the network payload report Information field does not gZC2PanID, and gZC3 PanID.
10. Upon network report command unicast message from DUT ZR2, gZC1 does not initiate network update 
command to inform about new PAN ID.
11. gZC1 over the air packet, in the MAC frame header destination
PanID is not set as the old PanID.
12. gZC1 over the air packet, in the MAC and NWK frame header source address does not same as 0x0000.
13. gZC1 over the air packet, in the MAC and NWK frame header destination addres does not same as 0xffff.
14. gZC1 over the air packet, in the network payload command frame identifier does not equal to 0x0a.
15. gZC1 over the air packet, in the network payload update information count sub-field of the command 
options field does not equal to 01
16. gZC1 over the air packet, in the network payload update Command Identifier sub-field of the command 
options field does not contain the value 0x00.
17. gZC1 over the air packet, in the network payload EPID field does not 0xaa aa aa aa aa aa aa aa.
18. gZC1 over the air packet, in the network payload update Information field does not contain the ZC1 new PanID.
19. gZC1 does not broadcast over-the-air APS Data frame to all nodes.
    DstAddrMode=0x02=16-bit
    DstAddr=0xffff=broadcast
    DstEndpoint=0xF0
    ProfileId=0x7f01=Test profileID
    ClusterID=0x001c
    SrcEndpoint=0x01=endpoint 1 on DUT ZC1
    asduLength=length of test MSG Buffer_Test_req
    asdu=Buffer_Test_req with sequence length octet (10)
20. DUT ZC1 over the air packet, in the MAC frame header destination PanID is not set as the new PanID.
21. DUT ZR2 does not transmit over-the-air APS Data frame to gZC1.
    DstAddrMode=0x02=16-bit
    DstAddr=0x0000=DUT ZC1
    DstEndpoint=0x01=endpoint 1 of DUT ZC1
    ProfileId=0x7f01=Test profileID
    ClusterID=0x0054
    SrcEndpoint=0xF0=endpoint 240 on DUT ZR2
    asduLength=length of test MSG Buffer_Test_rsp
    asdu=Buffer_Test_rsp with sequence length octet (10)
22. DUT ZR2 over the air packet, in the MAC frame header destination PanID is not set as the new PanID.

****
ZC1:
after 75sec. timeout sends broadcast test buffer request;
ZC2:
after startup completition change pan id;
after 10sec. timeout shutdown;
ZR2:
-
ZR3:
-
****

Comments:
4 devices: ZC1, ZR2, ZC2, ZR3.
Visibility:
ZC1 - ZR2 - ZC2 - ZR3.
- ZC1 starts.
- ZR2 joins it.
- ZC2 starts choosing panid as usual.
As a side effect is remembers ZC1's panid in the external variable
(special hack for this test).
- After start ZC2 changes PANID to be equal to ZC1's panid and updates beacon payload
- ZR3 tries to join and issues beacon request.
ZR3 will fail to join bacause of NS limitations (NS supports onlt 1 ZC), but it is not
problem in this test
- ZR2 sees ZC2's beacon and initiates PANID conflict resolution.
