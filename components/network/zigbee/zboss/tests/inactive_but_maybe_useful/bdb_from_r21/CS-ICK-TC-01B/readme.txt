7.1.2 CS-ICK-TC-01B: Network formation test, a device joins, using IC-based LK; DUT: joiner (ZR/ZED)
This test covers joining centralized network with the IC-based link key; incl. network key delivery and TC-LK update. 
This test verifies the operation of the device attempting to join a centralized network with IC-based link key.
The DUT has to perform the role specified in its PICS; the other role can be performed by a test harness or a golden unit.


Required devices:
DUT - ZR/ZED, capable of joining a centralized network and having IC
THc1 - TH ZC, capable of centralized network formation
THc1 is capable of accepting ICs 


Preparatory steps:
P1 - THc1 is powered on and triggered to form a centralized network. THc1 successfully forms a centralized network. 
P2a - IC-derived TCLK of the DUT is entered into THc1, together with DUT’s IEEE address. 
      Network steering is triggered on the THc1.
      The IC value SHALL be different from the exemplary value provided in the BDB specification.
P2b - DUT is powered on. Network steering is triggered on the DUT. DUT successfully completes the Beacon request, Beacon, Association Request, and Association Response with THc1.
The test steps have to follow while the AssociationPermit of the THc1 is enabled (i.e. within bdbcMinCommissioningTime).


Test Procedure:
 - THc1 sends NWK key to DUT: Within apsSecurityTimeOutPeriod after sending the successful MAC Association Response frame to the DUT, THc1 unicasts to the DUT an APS Transport Key command: unprotected at the NWK level, protected at the APS layer with the IC-derived Key Transport Key.
 - THc1 unicasts ZDO Node_Desc_rsp to DUT: Within bdbTrustCenterNodeJoinTimeout after transmission of the network key, THc1 unicasts to DUT the ZDO Node_Desc_rsp (ClusterID = 0x8002), with: Status – SUCCESS; NWKAddrOfInterest = 0x0000; Node Descriptor with: Server mask parameter indicating core spec release r21 or later.
 - THc1 responds with a correctly formatted and protected APS Transport Key command, carrying a unique TC-LK for DUT: Within bdbTCLinkKey- ExchangeTimeout THc1 unicasts to DUT a correctly formatted APS Transport Key command: correctly protected at the NWK level with the NWK key, and correctly protected at the APS level with the key-load-key derived from the IC.
 - THc1 responds with APS Confirm-Key command:
   THc1 unicasts to DUT the APS Confirm-Key command:
   correctly protected at the NWK level with the NWK key; 
   correctly protected at the APS level with the data key derived from the new unique TC-LK.


Additional info:
to run test type - runng <dut_role>
where dut_role can be  zed or zr
e.g: runng zr
