3.5.5 DN-NFS-TC-05: Network formation followed by steering on factory-new ZR; DUT: ZR

This test verifies the network formation and steering procedure.
This test verifies the behavior of the factory-new ZR (bdbNodeIsOnANetwork = FALSE) when network formation automatically followed by network steering is triggered.  It verifies that the DUT performs both actions correctly and in the correct order.
It’s an optional test, only to be executed if the DUT’s PICS indicates support for such combination of commissioning procedures.
Successful completion of the applicable tests from sec. Distributed network formation and Distributed network steering is a condition for performing the current test.

Initial Conditions:
1) All devices are not operational on a network (for information: bdbNodeIsOnANetwork = FALSE) and switched off until used.
2) There are no open networks on the primary and secondary channels supported by the DUT.


Test procedure:
1a
Before: Power DUT on.
After: DUT is powered on.

1b
Before: In DUT-specific way network formation automatically followed by steering is triggered on the DUT.
After: DUT performs a scan to find a suitable channel.

2
Before: Zigbee PRO/2007 Layer PICS and Stack Profiles, Zigbee Alliance document 08-0006r05 or later. TCC2
DUT forms its own network.  
Wait for at most 15 seconds.
After: Within 15 seconds, DUT starts sending Link Status commands.
Note: since the NWK key of DUT’s network was NOT sent over the air, the payload of the command likely cannot be decrypted by the sniffer.

3a
Before: DUT automatically (i.e. without user interaction) enables network steering.
After: DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s); DUT’s short address is NOT equal to 0x0000.
Note: since the NWK key of DUT’s network was NOT sent over the air, the payload of the command likely cannot be decrypted by the sniffer.
For information: ZC sets own AssociationPermit to TRUE. This will be verified by the following step.

3b
Before: Power on THr1. 
If required, network steering is triggered on THr1.
THr1 broadcasts at least on the operational channel of the DUT a Beacon request.
After: DUT unicasts to the THr1 a Beacon with AssociationPermit = TRUE.

