4.2.2 CN-CNF-TC-02: Network formation on a non-factory-new ZC, DUT: ZC

This is the negative test to check that a ZC that formed a centralized network (bdbNodeIsOnANetwork = TRUE) does not create another network upon application trigger. 
1This test shall only be executed if the DUT does not automatically perform a reset when network formation is re-triggered, which will be indicated in device PICS. 
This test verifies the operation of the device that formed the centralized network (ZC); the other role can be performed by a golden unit or test harness. 


Required devices:
DUT - ZC, capable of centralized network formation
THr1 - TH ZR
this role can be performed by a golden unit device or a TH
Additional devices
THr2 - TH ZR (to make sure ZC will send Link Status messages)
this role can be performed by a golden unit device or a TH


Test preparation:
P1 DUT is triggered to form a centralized network. Centralized network is successfully formed.
P2 Network steering is triggered on the DUT. Network steering is triggered on THr1. It successfully joins the centralized network of DUT, incl. successful NWK key exchange and TC-LK update. 
Wait until network steering procedure closes (PermitDuration expires).


Test Procedure:
1a
Before: DUT is operational on a network.
After: DUT broadcasts Link Status messages.

1b
Before:Switch THr2 on.
Network steering is triggered on the THr2. 
THr2 broadcasts at least one MAC Beacon Request command at least on the operational network channel of the DUT.
After: DUT responds with Beacon frame with AssociationPermit = FALSE:
DUT unicasts to the THr2 a Beacon frame with MAC source PANId field set to the current PANId of the DUT, MAC source address field set to the NWK address of the DUT, and the payload carrying the current EPID; the AssociationPermit = FALSE.

2a
Before: Negative test: 
Network formation is triggered on the DUT. 
After: DUT continues operation on the originally formed centralized network.
Wait for at most 15 seconds.
Within the 15 seconds, DUT starts broadcasting Link Status messages, with PANId field set to the PANId of step 1a, MAC source address field set to the NWK address of the DUT as in step 1a.

2b
Before: THr2 broadcasts at least one MAC Beacon Request command at least on the operational network channel of the DUT.
After: DUT unicasts to the THr2 a Beacon frame with MAC source PANId field set to the PANId of step 1, MAC source address field set to the NWK address of the DUT as in step 1b, and the payload carrying the  EPID of step 1b.
