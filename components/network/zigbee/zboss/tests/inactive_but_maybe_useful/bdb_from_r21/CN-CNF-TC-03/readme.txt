4.2.3 CN-CNF-TC-03: Network formation on a non-factory-new ZR

This is the negative test to check that a ZR that joined a centralized network (bdbNodeIsOnANetwork = TRUE) does not create a distributed network upon application trigger. 
1This test is only applicable to DUT-ZR which support for application trigger for performing network formation only (i.e. not combined with reset and network forming or steering). Otherwise, this test shall not be performed.
This test verifies the operation of the device that joined the centralized network (ZR); the other role can be performed by a golden unit or test harness.


Required devices:
DUT - ZR  capable of distributed network formation
THr1 - TH ZRthis role can be performed by a golden unit device or a TH
Additional devices
THc1 - TH ZC capable of centralized network formation
This role can be performed by a golden unit or a TH


Preparatory steps:
P1 - THc1 is triggered to form a centralized network. Centralized network is successfully formed on a channel supported by the DUT.
P2 - DUT is triggered to join a network. It successfully joins the centralized network of THc1, incl. successful NWK key exchange and TC-LK update.
P3 - Association permit of DUT and THc1 are set to false (e.g. because of the steering procedure of P2 times out).


Test Procedure:
1a
Before: DUT is operational on a network.

After: DUT broadcasts Link Status messages on the operational channel and with the PANId of the THc1’s network.

1b
Before: Switch THr1 on.
Network steering is triggered on the THr1. 
THr1 broadcasts at least one MAC Beacon Request command at least on the operational network channel of the DUT.

After: DUT responds with a Beacon frame with AssociationPermit = FALSE: 
DUT unicasts to the THr1 a Beacon frame with MAC source PANId field set to the current PANId of the DUT, MAC source address field set to the NWK address of the DUT, and the payload carrying the current EPID; the AssociationPermit = FALSE.

2a
Before: Negative test: 
Network formation is triggered on the DUT.

After: DUT continues operation on the originally joined centralized network of THc1.
Wait for at most 15 seconds.
Within those 15 seconds, DUT starts broadcasting Link Status messages, with PANId field set to the PANId of step 1b, MAC source address field set to the NWK address of the DUT as in step 1b.

2b
Before: 1If required, network steering is triggered again on the THr1. 
THr1 broadcasts at least one MAC Beacon Request command at least on the operational network channel of the DUT.

After: DUT unicasts to the THr1 a Beacon frame with MAC source PANId field set to the PANId of step 1b, MAC source address field set to the NWK address of the DUT as in step 1, and the payload carrying the  EPID of step 1b.
