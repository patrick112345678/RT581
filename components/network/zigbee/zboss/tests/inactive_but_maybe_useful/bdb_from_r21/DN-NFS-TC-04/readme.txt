3.5.4 DN-NFS-TC-04: Formation&steering triggered on a NFN node
This test verifies the network formation and steering procedure.
This test verifies the behavior of a not-factory-new ZR (bdbNodeIsOnANetwork = TRUE) when network formation automatically followed by network steering is triggered.  It verifies that a not-factory-new router skips the network formation step and proceeds directly to network steering. 
It’s an optional test, only to be executed if the DUT’s PICS indicates support for such combination of commissioning procedures.
Successful completion of the applicable tests from sec. Distributed network formation and Distributed network steering is a condition for performing the current test.

Devices:
DUT - ZR, capable of distributed network formation and joining, also triggered at the same time.
THr1 - TH ZR

Test preparation:
P1 -Power DUT on. Trigger network formation on the DUT. DUT successfully forms a network. If the DUT allowed for network steering (AssociationPermit=TRUE), wait for the steering expire.
P2 -The PANId of DUT’s network and DUT’s short address are known (e.g. from Link Status messages, if sent by the DUT, or from Beacons sent in response to Beacon Request).


Test Procedure:
1a
Before: Network formation&steering is triggered on the DUT.
After: For information: DUT does NOT form a new network. 
       DUT does NOT send Beacon Requests.
       DUT opens the network for joining.
       DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).

       (For information: since no node joined the network of the DUT, the sniffer may not be able to decrypt the message.  DUT’s behaviour will be verified in step 1b).

1b
Before: Switch THr1 on. THr1 sends at least one MAC Beacon Request at least on the operational channel of the DUT’s network.
After: DUT responds to Beacon request with Beacon with AssociationPermit = TRUE, with PANId and short address as in preparatory step P2.




