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
P1a	DUTis powered on and triggered to establish a network. DUT forms a network; if the DUT is a ZC then a centralized network will be formed; if the DUT is a ZR then a distributed network will be formed. 
Network steering is enabled on the DUT. 
P1b	Within the PermitDuration of the DUT from step P1a, THe1 is powerd on and network steering is enabled. THe1 joins the network formed by the DUT in P1a.
P2	Network is closed (AssociationPermit of DUT is set to FALSE, e.g. because it expires). There are no other open networks on the operational channel of the DUT’s network.
P3	If ZC is the DUT, DUT has TC policy allowRemoteTcPolicyChange = TRUE. DUT’s TC policy value for useWhiteList is known. 
If useWhiteList = TRUE, the information about THr1 and THe1 has to be entered into the apsDeviceKeyPairSet of the DUT-ZC.
P4	THr1 is powered on.

Test Procedure: before
Reception of Mgmt_Permit_Joining_req from a ZED
1a	Network steering is triggered on THe1:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration of at least bdbcMinCommissioningTime (180s) and TC_Significance = 0x01.	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
1b	To check the AssociationPermit values, THr1 sends a Beacon request:
If required, network steering is triggered on THr1. 
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.

Reception of Mgmt_Permit_Joining_req with PermitDuration = 0x00
If THr1 joined the network in step 1b, reset THr1 to factory new.
2a	Within the PermitDuration of step 1a, THe1 sends Mgmt_Permit_Joining_req with PermitDuration 0x00:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.	DUT re-broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.
2b	THr1 sends a Beacon request to check the AssociationPermit values:
If required, network steering is triggered on THr1.
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.

Short PermitDuration (shorter than bdbcMinCommissioningTime)
3a	THe1 sends Mgmt_Permit_Joining_req with PermitDuration of 10 sec:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0x0a and TC_Significance = 0x01.	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
3b	To check the AssociationPermit values, within PermitDuration, the THr1 sends a Beacon request:
If required, network steering is triggered on THr1.
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.
3c	Wait for PermitDuration expiration.	For information: at PermitDuration expiration, DUT disables its own AssociationPermit.
3d	If THr1 joined the network in step 3b, reset THr1 to factory new.
Note: this can be performed while waiting for PermitDuraiton expiration.	
3e	To check the AssociationPermit values, THr1 sends a Beacon request: 
If required, network steering is triggered on THr1.
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.

Long PermitDuration  (longer than bdbcMinCommissioningTime)
4a	THe1 sends Mgmt_Permit_Joining_req with PermitDuration = 0xFE:
THe1 broadcasts to 0xfffc (unicast via the parent) a Mgmt_Permit_Joining_req with PermitDuration = 0xFE and TC_Significance = 0x01.	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
4b	Wait for bdbcMinCommissioningTime to expire.	
4c	To check the AssociationPermit values, THr1 sends a Beacon request:
If required, network steering is triggered on THr1.
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.

Optional test: Disabling of network steering via local application trigger
If not supported: ZED broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00.
If THr1 joined the network in step 4c, reset THr1 to factory new.
5a	Conditional on DUT supporting local network steering disabling:
Network steering is disabled on the DUT.	For information: DUT disables own AssociationPermit.
5b	To check the AssociationPermit values, THr1 sends a Beacon request:
If required, network steering is triggered on THr1.
THr1 broadcasts on the operational channel of the network a Beacon request frame.	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.


Test Procedure: after
1a	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
1b	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.

2a	DUT re-broadcasts Mgmt_Permit_Joining_req with PermitDuration = 0x00 and TC_Significance = 0x01.
2b	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.

3a	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
3b	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.

3c	For information: at PermitDuration expiration, DUT disables its own AssociationPermit.
3d	
3e	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.

4a	DUT re-broadcasts Mgmt_Permit_Joining_req with the same PermitDuration and TC_Significance = 0x01.
4b	
4c	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = TRUE.

5a	For information: DUT disables own AssociationPermit.
5b	DUT unicasts to the THr1 a Beacon frame with AssociationPermit = FALSE.


