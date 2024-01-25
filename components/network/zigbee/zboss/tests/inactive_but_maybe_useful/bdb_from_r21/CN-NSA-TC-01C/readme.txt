4.5.3 CN-NSA-TC-01C: Mgmt_Permit_Joining_req handling:
This test verifies the behavior on the reception of Mgmt_Permit_Joining_req on a DUT, which can be a ZR or a ZC. 
The DUT shall take the role defined in its PICS; the other role can be performed by a golden unit or test harness, as indicated.


Required devices:
DUT - ZC or ZR

Additional devices:
THe1 - TH ZED 
this role can be performed by a TH
THr1 - TH ZR 


Test preparation:
Preparatory steps:
P1 - DUT, THr1 and THe1 are powered on and triggered to establish a network. DUT forms a network; if the DUT is a ZC then a centralized network will be formed; if the DUT is a ZR then a distributed network will be formed. Then, THr1 and THe1 join the network of the DUT.
P2 - Network is closed (AssociationPermit of DUT and THr1 is set to FALSE, e.g. because it expires).
P3 - If ZC is the DUT, the TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT-ZC.



Test Procedure: before

Reception of Mgmt_Permit_Joining_req from a ZED
1a - Network steering is triggered on THe1:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.
1b - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.


Reception of Mgmt_Permit_Joining_req with PermitDuration = 0x00
2a - Within the PermitDuration of step 1a, THe1 sends Mgmt_Permit_Joining_req with PermitDuration 0x00:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.
2b - THe1 sends a Beacon request to check the AssociationPermit values:
THe1 broadcasts on the operational channel of the network a Beacon request frame.


Short PermitDuration (shorter than bdbcMinCommissioningTime)
3a - THe1 sends Mgmt_Permit_Joining_req with PermitDuration of 10 sec:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0x0a and TC_Significance = 0x01.
3b - To check the AssociationPermit values, within PermitDuration, the THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.
3c - Wait for PermitDuration expiration.
3d - To check the AssociationPermit values, THe1 sends a Beacon request: THe1 broadcasts on the operational channel of the network a Beacon request frame.


Long PermitDuration  (longer than bdbcMinCommissioningTime)
4a - THe1 sends Mgmt_Permit_Joining_req with PermitDuration = 0xFE:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0xFE and TC_Significance = 0x01.
4b - Wait for bdbcMinCommissioningTime to expire.
4c - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.


Optional test: Disabling of network steering via local application trigger
If not supported: ZED broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00.
5a - Conditional on DUT supporting local network steering disabling:
Network steering is disabled on the DUT.
5b - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.


Unicast Mgmt_Permit_Joining_req
6a - THe1 unicasts Mgmt_Permit_Joining_req to the THr1:
THe1 unicasts to the THr1 a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.
6b - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.
6c - THe1 unicasts Mgmt_Permit_Joining_req to the DUT:
THe1 unicasts to the DUT a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.
6d - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.
6e - THe1 sends Mgmt_Permit_Joining_req to the DUT with PermitDuration = 0x00: THe1 unicasts to the DUT a Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.
6f - To check the AssociationPermit values, THe1 sends a Beacon request:
THe1 broadcasts on the operational channel of the network a Beacon request frame.
6g - THe1 sends Mgmt_Permit_Joining_req to THr1:
THe1 unicasts to THr1 (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration =0x00 and TC_Significance = 0x01.



Test Procedure: after

Reception of Mgmt_Permit_Joining_req from a ZED
1a - DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
1b - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.

Reception of Mgmt_Permit_Joining_req with PermitDuration = 0x00
2a - DUT re-broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.
2b - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.


Short PermitDuration (shorter than bdbcMinCommissioningTime)
3a - DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
3b - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.
3c - For information: at PermitDuration expiration, DUT disables its own AssociationPermit.
3d - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.


Long PermitDuration  (longer than bdbcMinCommissioningTime)
4a - DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
4b - empty
4c - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.

Optional test: Disabling of network steering via local application trigger
If not supported: ZED broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00.
5a - For information: DUT disables own AssociationPermit.
5b - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.


Unicast Mgmt_Permit_Joining_req
6a - THr1 sends Mgmt_Permit_Joining_rsp:
THr1 unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: THr1 enables own AssociationPermit for PermitDuration.
6b - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.
THr1 unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.
6c - DUT sends Mgmt_Permit_Joining_rsp:
DUT unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: DUT enables own AssociationPermit for PermitDuration.
6d - DUT unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.
THr1 unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.
6e - DUT unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: DUT disables own AssociationPermit.
6f - THr1 unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.
DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.
6g - THr1 unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.

