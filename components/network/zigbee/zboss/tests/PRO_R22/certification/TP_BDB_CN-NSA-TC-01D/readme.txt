CN-NSA-TC-01D: Handling of unicast Mgmt_Permit_Joining_req 
This test verifies the behavior on the reception of unicast Mgmt_Permit_Joining_req on a DUT, which can be a ZR or a ZC. 
This test is only applicable to DUT-ZC with Trust Centers supporting allowRemoteTcPolicyChange = TRUE, otherwise it shall not be performed.
The DUT shall take the role defined in its PICS; the other role can be performed by a golden unit or test harness, as indicated.

Required devices:
DUT - ZC or ZR

Additional devices:
THe1 - TH ZED 
this role can be performed by a TH
THr1 - TH ZR 
THr2 - TH ZR 

Test preparation:
Preparatory steps:
P1 - DUT, THr1 and THe1 are powered on and triggered to establish a network. DUT forms a network; if the DUT is a ZC then a centralized network will be formed; if the DUT is a ZR then a distributed network will be formed. Then, THr1 and THe1 join the network of the DUT.
P2 - Network is closed (AssociationPermit of DUT and THr1 is set to FALSE, e.g. because it expires).
P3 - If ZC is the DUT, the TC policy values of allowRemoteTcPolicyChange and useWhiteList are known. If allowRemoteTcPolicyChange = FALSE and/or useWhiteList = TRUE, Mgmt_Permit_Join_Req will not be accepted by the DUT-ZC.
P1a	DUTis powered on and triggered to establish a network. DUT forms a network; if the DUT is a ZC then a centralized network will be formed; if the DUT is a ZR then a distributed network will be formed. 
Network steering is enabled on the DUT. Then, THr1 and THe1 join the network of the DUT.
P1b	Within the PermitDuration of the DUT from step P1a, THr1 is powerd on and network steering is enabled. THr1 joins the network formed by the DUT in P1a.
P1c	Within the PermitDuration of the DUT from step P1a, THe1 is powerd on and network steering is enabled. THe1 joins the network formed by the DUT in P1a.
P2	Network is closed (AssociationPermit of DUT and THr1 is set to FALSE, e.g. because it expires).
P3	If ZC is the DUT, DUT has TC policy allowRemoteTcPolicyChange = TRUE. DUT’s TC policy value for useWhiteList is known. 
If useWhiteList = TRUE, the information about THr1, THe1 and THr2 has to be entered into the apsDeviceKeyPairSet of the DUT-ZC.
P4	THr2 is powered on.

Test Procedure: before
1a	THe1 unicasts Mgmt_Permit_Joining_req to the THr1:
THe1 unicasts to the THr1 a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.

1b	To check the AssociationPermit values, THr2 sends a Beacon request:
If required, network steering is triggered on THr2.
THr2 broadcasts on the operational channel of the network a Beacon request frame.
If THr2 joined the network in step 1b, reset THr2 to factory new.

1c	THe1 unicasts Mgmt_Permit_Joining_req to the DUT:
THe1 unicasts to the DUT a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.

1d	To check the AssociationPermit values, THr2 sends a Beacon request:
If required, network steering is triggered on THr2.
THr2 broadcasts on the operational channel of the network a Beacon request frame.
If THr2 joined the network in step 1d, reset THr2 to factory new.

1e	THe1 sends Mgmt_Permit_Joining_req to the DUT with PermitDuration = 0x00: THe1 unicasts to the DUT a Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.

1f	To check the AssociationPermit values, THr2 sends a Beacon request:
If required, network steering is triggered on THr2.
THr2 broadcasts on the operational channel of the network a Beacon request frame.


Test Procedure: after

1a	For information: THr1 sends Mgmt_Permit_Joining_rsp:
THr1 unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: THr1 enables own AssociationPermit for PermitDuration.

1b	DUT unicasts to the THr2 a Beacon frame with AssociationPermit = FALSE.
For information: THr1 unicasts to the THr2 a Beacon frame with AssociationPermit = TRUE.

1c	DUT sends Mgmt_Permit_Joining_rsp:
DUT unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: DUT enables own AssociationPermit for PermitDuration.

1d	DUT unicasts to the THr2 a Beacon frame with AssociationPermit = TRUE.
For information: THr1 unicasts to the THr2 a Beacon frame with AssociationPermit = TRUE.

1e	DUT unicasts to THe1 a Mgmt_Permit_Joining_rsp, with the Status SUCCESS.
For information: DUT disables own AssociationPermit.

1f	DUT unicasts to the THe1 a Beacon frame with AssociationPermit = FALSE.
For information: THr1 unicasts to the THe1 a Beacon frame with AssociationPermit = TRUE.



