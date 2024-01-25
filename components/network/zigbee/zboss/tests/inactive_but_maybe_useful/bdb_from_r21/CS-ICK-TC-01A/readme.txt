7.1.1 CS-ICK-TC-01A: Network formation test, a device joins, using IC-based LK; DUT: ZC
This test covers joining centralized network with the IC-based link key; incl. network key delivery and TC-LK update. 
This test verifies the behavior of the ZC accepting the joiner with IC-based link key.
The DUT has to perform the role specified in its PICS; the other role can be performed by a test harness or a golden unit.



Required devices:
THr1 - TH ZR, capable of joining a centralized network and having IC
DUT - ZC, capable of centralized network formation
      ZC is capable of accepting ICs 



Preparatory steps:
P1 - DUT is powered on and triggered to form a centralized network. DUT successfully forms a centralized network. 
P2a - IC-derived TCLK of the THr1 is entered into DUT, together with THr1’s IEEE address. 
Network steering is triggered on the DUT.
The IC value SHALL be different from the exemplary value provided in the BDB specification.
P2b - THr1 is powered on. Network steering is triggered on the THr1. THr1 successfully completes the Beacon request, Beacon, Association Request, and Association Response with the DUT.
The test steps have to follow while the AssociationPermit of the DUT is enabled (i.e. within bdbcMinCommissioningTime).
