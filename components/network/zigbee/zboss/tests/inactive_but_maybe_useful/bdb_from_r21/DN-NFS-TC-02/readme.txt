3.5.2 DN-NFS-TC-02: Distributed network steering & formation, FN device, open network; DUT: ZR:
This test verifies the network steering followed by formation procedure.
It’s an optional test, only to be executed if the DUT’s PICS indicates support for such combination of commissioning procedures.

This test verifies the behavior of the ZR when network steering and network formation are triggered simultaneously, the steering followed by formation.  Specifically, this test verifies, that such a device will refrain from forming its own network, when a suitable open network is found, and join the found network instead.
Successful completion of the applicable tests from sec. Distributed network formation and Distributed network steering is a condition for performing the current test.
The device takes the role described in its PICS; the other role can be performed by a test harness.

Devices:
DUT - ZR, capable of distributed network formation
THr1 - TH ZR, capable of distributed network formation on a pre-configured channel


Preparation steps:
P1 - THr1 is powered on and triggered to form a distributed network. THr1 successfully forms a distributed network on a channel of the DUT’s bdbPrimaryChannelSet if not 0x00000000, else on one of DUT’s bdbSecondaryChannelSet.


Test Procedure:
1a
Before: Trigger network steering on THr1.
After: For information:  the network of THr1 is open for joining.

1b
Before: Place DUT in 1m proximity of THr1 and power on DUT.
After: DUT is powered on.

1c
Before: Network steering and formation is triggered on the DUT. 
After: DUT performs a scan to find a suitable open network.

1d
Before: Since THr1’s network was formed on one of the DUT’s channels, THr1 receives a Beacon Request and unicasts to the DUT a Beacon frame with AssociationPermit =TRUE.
After: DUT does NOT decide to form its own network, due to the enabled network steering and the presence of an open network.
DUT performs successfully association procedure with the THr1, and receives the network key.

2a
Before: DUT becomes operational on the network of THr1.
After: DUT sends correctly protected Device_annce.
       DUT starts sending Link Status messages.
2b
Before: DUT broadcasts to 0xfffc a correctly protected Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s).
For information: DUT sets own AssociationPermit to TRUE.
After: THr1 re-broadcasts the Mgmt_Permit_Joining_req.
       Note: the behaviour of a device upon sending of Mgmt_Permi_Join upon completed steering is tested in CN-NSA-TC-01A and -01B.
