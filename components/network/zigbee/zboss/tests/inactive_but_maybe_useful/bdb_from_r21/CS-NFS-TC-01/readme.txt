7.2.1 CS-NFS-TC-01: Negative test: TC-LK update; DUT: ZR/ZED

Required devices:
DUT - DUT capabilities according to its PICS: ZED or ZR 

THc1 - TH in the role of Zigbee PAN Coordinator and Trust Centre
       THc1 has TC policy values of allowRemoteTcPolicyChange =TRUE and useWhiteList =FALSE.


Initial conditions:
1 - A packet sniffer shall be observing the communication over the air interface.
2 - For information: for the DUT, bdbTCLinkKeyExchangeMethod = 0x00.
    The value of the DUT’s bdbTCLinkKeyExchangeAttemptsMax attribute is known (default = 0x3).


Preparatory steps:
P0a - THc1 is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P0b - Network formation is triggered on THc1.
      THc1 successfully completes formation of a centralized network.
P1 - The DUT is factory new (For information: bdbNodeIsOnANetwork = FALSE).
P2a - Network steering is triggered on the THc1. THc1 broadcasts Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime and TC_Significance = 0x01.
P2b - Network steering is triggered on the DUT.
P3 - DUT and THc1 successfully complete the exchange of Beacon request, Beacon, Association Request, Association response.  
P4a - THc1 delivers the NWK key to the DUT in unicast APS Transport Key command, correctly protected with the default global Trust Center link key; and the value of the APS frame counter field is noted.  
P4b - For information: DUT stores the network key in the NIB and the apsTrustCenterAddress to the IEEE address of the DUT-ZC, as indicated in the APS Transport Key command.
DUT creates an entry in the apsDeviceKeyPairSet of the AIB, with the DeviceAddress parameter set to the IEEE address of the THc1 and the LinkKey parameter set to the default global TC-LK; the outgoing frame counter is set to 0x00; the incoming frame counter is set to the value used by the THc1 in the Transport Key command.
P4c - DUT sends, correctly protected with the network key, a broadcast ZDO Device_annce command, and starts broadcasting to 0xfffc a NWK Link Status command.
P4d - DUT sends to THc1 a Node_Desc_req. THc1 responds with Node_Desc_rsp; server mask indicates r21 or later.
P4e - For information: DUT sets bdbTCLinkKeyExchangeAttempts to 0x00.


Additional info:
 Test consists of 16 test stages.
 After each test stage reboot devices.
 Enable macro USE_NVRAM_IN_TEST to allow DUT update it's test state after reboot.
