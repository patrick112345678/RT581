4.5.1 CN-NSA-TC-01A: Mgmt_Permit_Joining_req sending on centralized network
This test verifies the behavior of a DUT ZR after sending Mgmt_Permit_Joining_req upon successfully joining a centralized network. 
The DUT shall take the role defined in its PICS; the other role can be performed by a golden unit or test harness, as indicated.


Required devices:
DUT - ZR

Additional devices:
THe1 - TH ZED 
THc1 - TH ZC 


Test preparation:
P1 - THc1 is triggered to establish a centralized network on one of the primary channels supported by the DUT.
P2 - Network is closed (AssociationPermit of THc1 is set to FALSE, e.g. because it expires).


Test Procedure:
Generation of Mgmt_Permit_Joining_req upon completed steering
1a
Before: Network steering is triggered on THc1.
DUT is brought in radio range of THc1 and powered on. 
Network steering is triggered on the DUT.
After: DUT successfully performs network sttering with the THc1.
DUT broadcasts to 0xfffc a Mgmt_Permit_Joining_req with PermitDuration = 0xb4 (180s) and TC_Significance = 0x01.
2a
Before:THe1 is brought in radio range of DUT and THc1 and powered on. 
Network steering is triggered on the THe1.
After: On reception of the Beacon Request from the THe1, DUT unicasts to the THe1 a Beacon frame with AssociationPermit=TRUE.
If THc1 also responds with a Beacon to THe1, it also has AssociationPermit=TRUE.
2b
After: THe1 may complete network steering, with the DUT, or with THc1 – if THc1 responded with a Beacon in step 2a.  
