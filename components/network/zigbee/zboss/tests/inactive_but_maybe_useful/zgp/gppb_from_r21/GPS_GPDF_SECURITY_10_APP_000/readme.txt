Test described in GP test specification, clause 4.2.3 Security and 4.2.3.2 SecurityLevel = 0b10 and AppID = 0b000

Test Harness tool - as ZR
Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network

TH-GPD and DUT-GPS have a pairing with 
    ApplicationID = 0b000, 
    SrcID = A, 
    SecurityLevel = 0b10 and any shared key type (gpSharedSecurityKeyType 0b001, 0b010 or 0b011); 
    the key value (gpSharedSecurityKey) and SecurityFrameCounter value are known.

4.2.3.2 Test procedure
commands sent by TH-tool and TH-gpd:

1: 
- TH-GPD sends Data GPDF, with the Extended NWK Frame Control
  field present, ApplicationID = 0b000, SrcID = A, the SecurityLevel subfield set to 0b10, 
  SecurityKey set to 0b0, the SecurityFrameCounter field present and set to correct value N, and the payload correctly 
  authenticated with 4B MIC.

- Th-Tool - Read out Sink Table of DUT-GPS.
  
2: Negative test (wrong key):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = A, protected with a correct security 
  level (0b10), correct frame counter value (=M>=N+1), correct key type (0b0) but wrong key.

3: Negative test (wrong key type):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = A, protected with a correct security 
  level (0b10), correct frame counter value (=P>=M+1), correct key but wrong key type (0b1).

!!!Changing key type with correct security key did not change the contents of packet, send by GPD 
   (all fields will be set as in normal packet, because key_type not transmitted in the packet).
   We will change SecurityKey bit in ExtNWK frame control field to individual (0b1) according Table 12.

4: Negative test (wrong security level):
- TH-GPD sends an unprotected Data GPDF with ApplicationID = 0b000, SrcID = A, with correct frame 
  counter value (=Q>=P+1). (SecurityLevel = 0b00).
  
5: Negative test (wrong security level):
- Opt. TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = A, security level lower than the 
  before (SecurityLevel = 0b01), with counter value =R>=Q+1.

6a:Negative test (replayed frame):
- TH-GPD sends a correctly secured replayed Data GPDF with ApplicationID = 0b000, SrcID = A, (with old 
  frame counter N-1).
  
6b:Positive test: 
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = A, correctly formatted and protected with Security frame 
  counter value =M>=N+1.

- Th-Tool - Read out Sink Table of DUT-GPS.

7a:Negative test (wrong frame type 0b10):
- Make TH-GPD send a correctly protected Data GPDF formatted as in 4.2.3.2.2/item 1 above 
  with the following changes: 
  - incremented Security frame counter, 
  - the Frame Type sub-field of the NWK Frame Control field set to 0b10.
  
7b:Negative test (wrong frame type 0b11):
- Make TH-GPD send a correctly protected Data GPDF formatted as in 4.2.3.2.2/item 1 above 
  with the following changes: 
  - incremented Security frame counter; 
  - the Frame Type sub-field of the NWK Frame Control field set to 0b11.

7c:Negative test (wrong frame type 0b01): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the Frame Type sub-field of the NWK Frame Control field set to 0b01.

7d:Negative test (Wrong Protocol Version):
- Make TH-GPD (or TH-Tool in the role of TH-GPD) send a correctly protected Data GPDF formatted 
  as in 4.2.3.2.2/item 1 above, with the following changes: 
  - incremented Security frame counter; 
  - wrong Zigbee Protocol Version sub-field of the NWK Frame Control field (0x2).

7e:Negative test (Auto-Commissioning and RxAfterTx both set to 0b1): 
- Make TH-GPD send Data GPDF formatted as in 4.2.3.2.2/item 1 above, with the following changes: 
  - incremented Security frame counter; 
  - AutoCommissioning field of NWK Frame Control field set to 0b1 and 
  - RxAfterTx field of Extended NWK Frame Control field is set to 0b1. 

7f:Negative test (malformed frame): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the Extended NWK Frame Control field is present and the NWK Frame Control Extension sub-field of the 
    NWK Frame Control field is set to 0b0.

7g:Negative test (malformed frame): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the Extended NWK Frame Control field is NOT present and the NWK Frame Control Extension sub-field of 
    the NWK Frame Control field is set to 0b1.

7h:Negative test (wrong ApplicationID): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b001.

7i:Negative test (wrong ApplicationID): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b011.

7j:Negative test (wrong Direction): 
- Make TH-GPD send a Data GPDF formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - the Direction sub-field of the Extended NWK Frame Control field is set to 0b1.
  
- Th-Tool - Read out Sink Table of DUT-GPS.

7K:Negative test: SrcID = 0x00000000: 
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send a correctly protected Data GPDF 
  formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - with ApplicationID = 0b000, SrcID = 0x00000000.

- Th-Tool - Read out Sink Table of DUT-GPS.

7L:Negative test: wrong SrcID: 
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send a correctly protected Data GPDF 
  formatted as in 4.2.3.2.2/item 1 above with the following changes: 
  - incremented Security frame counter; 
  - but with ApplicationID = 0b000, SrcID differing by one bit from A. 

- Th-Tool - Read out Sink Table of DUT-GPS.

8: Positive test: 
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = A, correctly formatted and protected 
  with Security frame counter value =L>=M+1.

- Th-Tool - Read out Sink Table of DUT-GPS.

Pass verdict:
1:  DUT-GPS successfully security processes the received GPDF, stores the new frame counter value in its Sink Table 
    entry for the SrcID =A and executes the GPD command.
    
2: 3: 4: 5: 6A: 
    DUT-GPS silently drops the GPDF without executing GPD command.
    DUT-GPS does not store the frame counter value in its Sink Table entry for the SrcID = A. 
    
6B: DUT-GPS successfully security processes the received GPDF, stores the new frame counter value in its Sink Table 
entry for the SrcID =A and executes the GPD command.

7A: 7B: 7C: 7D: 7E: 7F: 7G: 7H: 7I: 7J: 7K: 7L: 
    DUT-GPS silently drops the GPDF without executing GPD command.
    DUT-GPS does not store the frame counter value in its Sink Table entry for the SrcID = A.
    
8:  DUT-GPS successfully security processes the received GPDF, stores the new frame counter value in its Sink Table 
    entry for the SrcID =A and executes the GPD command.

Fail verdict:
1:  DUT-GPS does not execute the GPD command.
    AND/OR DUT-GPS does not store the new frame counter value in its Sink Table entry for the SrcID =A.
    
2: 3: 4: 5: 6A: 
    DUT-GPS does not silently drop the GPDF.
    AND/OR DUT-GPS executes the GPD command.
    AND/OR DUT-GPS stores the frame counter value in its Sink Table entry for the SrcID =A.
    
6B: DUT-GPS does not stores the new frame counter value in its Sink Table entry 
    for the SrcID =A and/or does not execute the GPD command.
    
7A: 7B: 7C: 7D: 7E: 7F: 7G: 7H: 7I: 7J: 7K: 7L:
    DUT-GPS does not silently drop the GPDF.
    AND/OR DUT-GPS executes the GPD command.
    AND/OR DUT-GPS stores the frame counter value in its Sink Table entry for the SrcID =A.
    
8:  DUT-GPS does not stores the new frame counter value in its Sink Table entry for 
    the SrcID =A and/or does not execute the GPD command.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
