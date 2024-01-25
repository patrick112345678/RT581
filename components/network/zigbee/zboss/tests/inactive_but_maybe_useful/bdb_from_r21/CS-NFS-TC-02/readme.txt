7.2.2 CS-NFS-TC-02: Negative test: TC-LK update; DUT: ZC requiring TC-LK update

This test covers the behavior of the Zigbee Coordinator with a Trust Centre, which requires TC-LK update (bdbTrustCenterRequireKeyExchange attribute is TRUE) and does NOT require IC (bdbJoinUsesInstallCodeKey attribute is FALSE).


Required devices:
DUT - ZC hosting the TC role, implementing the BDB. The DUT does NOT require ICs. 
THr1 - TH ZR; joined to the centralized network of the DUT
THr2 - TH ZR joining a centralized network of the DUT


Initial conditions
1 - A packet sniffer shall be observing the communication over the air interface.
2 - For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
    The value of the DUT’s bdbTrustCenterNodeJoinTimeout attribute is known (default = 0xf).
    The value of the DUT’s bdbTrustCenterRequireKeyExchange attribute is TRUE.
    The value of the DUT’s bdbJoinUsesInstallCodeKey attribute is FALSE.


Test preparation
P0 - The DUT, THr2 and THr1 are factory new (For information: bdbNodeIsOnANetwork = FALSE).
P1a - Network formation is triggered on DUT-ZC.
DUT successfully completes formation of a centralized network. (For information: bdbNodeIsOnANetwork = TRUE).
P1b - Network steering is triggered on the DUT. DUT broadcasts Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime and TC_Significance = 0x01.
P1c - THr1 is powered on and network steering is triggered on THr1.
THr1 successfully joins the centralized network of the DUT (incl. TC-LK update).
P2a - Network steering is triggered on the DUT. DUT broadcasts Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime and TC_Significance = 0x01.
P2b - Within AssociationPermit, network steering is triggered on the THr2.
P3 - DUT and THr2 successfully complete the exchange of Beacon request, Beacon, Association Request, Association response.  
For information: DUT starts the bdbTrustCenterNodeJoinTimeout  timer.
P4a - DUT delivers the NWK key to the THr2 in unicast APS Transport Key command, correctly protected with the default global Trust Center link key; and the value of the APS frame counter field is noted.  
P4b - For information: THr2 stores the network key in the NIB and the apsTrustCenterAddress to the IEEE address of the DUT-ZC, as indicated in the APS Transport Key command.
P4c - THr2 sends, correctly protected with the network key, a broadcast ZDO Device_annce command, and starts broadcasting to 0xfffc a NWK Link Status command.



Additional info:
Test takes about 40 minutes and consist of 9 stages.
DUT shall be started first and does not rebooted.
THr1 shall start second, join at DUT and does not rebooted.
THr2 starts last and joins to DUT.

After firsts start DUT performs steering on network.
Then every 4 minutes DUT retriggers Network Steering.
Just after DUT opens network THr2 joins to network, start test procedure according to
current stage then waits for 3 minutes and tries to update tclk again (or send APS command).
After next MgmtPermit Join from DUT (~4 minutes) THr2 reboots and goes to next test stage.

After Network Steering ends THr1 requests DUT's neighbor table and check it for THr2 response.
Both thr logging every test step and gives verdict (PASSED/FAILED).
Grep by "TEST:" to obtain this logs.
