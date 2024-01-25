Test described in GP test specification, clause 4.2.2 Basics and 4.2.2.1 ApplicationID = 0b000

Test Harness tool - as ZR
Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network

TH-GPD and DUT-GPS have a pairing with 
    ApplicationID = 0b000, 
    SrcID = A, 
    TH-GPD uses incremental MAC sequence number.
    A pairing is established between the TH-GPD and DUT-GPS
    (e.g. as a result of any of the commissioning procedures).
    Initial MAC sequence number is Z.
Note: Only basic GPDF frame format reception is tested here.
      Filtering of the GPD ID is tested separately.
      Duplicate filtering (based on GPD’s Sequence number capabilities) to be tested separately.
      Rx capabilities are tested separately, with bidirectional operation.
      Security is also tested separately.

4.2.2.1 Test procedure
commands sent by TH-tool and TH-gpd:

1: 
- TH-GPD send a Data GPDF with:
    MAC Sequence number W >= Z+1;
    NWK Frame Control field with the following values: 
    the Frame type sub-field set to 0b00, 
    the Zigbee Protocol Version sub-field set to 0b0011.
    the NWK Frame Control Extension sub-field set to 0b1; 
    the Extended NWK Frame Control field is present, with the following values:
    the ApplicationID sub-field set to 0b000, 
    the Direction sub-field set to 0b0.
    Only one of Auto-Commissioning field of NWK Frame Control field and 
    RxAfterTx field of Extended NWK Frame Control field is set to 0b1.
- Th-Tool - Read out Sink Table of DUT-GPS.

2:
- TH-GPD send a Data GPDF with:
    MAC Sequence number Q >= W+1;
    NWK Frame Control field with the following values: 
    the Frame type sub-field set to 0b00, 
    the Zigbee Protocol Version sub-field set to 0b0011.
    the NWK Frame Control Extension sub-field set to 0b0; 
    Extended NWK Frame Control field absent.
- Th-Tool - Read out Sink Table of DUT-GPS.

3a:Negative test (wrong frame type 0b10):
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
    incremented MAC Sequence number;
    the Frame Type sub-field of the NWK Frame Control field set to 0b10.

3b:Negative test (wrong frame type 0b11):
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
    incremented MAC Sequence number;
    the Frame Type sub-field of the NWK Frame Control field set to 0b11.
    
3c:Negative test (wrong frame type 0b01):
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
    incremented MAC Sequence number;
    the Frame Type sub-field of the NWK Frame Control field set to 0b01.

3d:Negative test (Wrong Protocol Version):
- TH-GPD send a correctly protected Data GPDF formatted as in item 1 above, with the following changes:
    incremented MAC Sequence number;
    wrong Zigbee Protocol Version sub-field of the NWK Frame Control field (0x2).

4: Negative test (malformed frame): 
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
    incremented MAC Sequence number;
    the Extended NWK Frame Control field is present and the NWK Frame Control Extension sub-field of the 
    NWK Frame Control field is set to 0b0.

5: Negative test (malformed frame): 
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    the Extended NWK Frame Control field is NOT present and the NWK Frame Control Extension sub-field of 
    the NWK Frame Control field is set to 0b1.

6: Negative test (wrong ApplicationID): 
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b001.

7: Negative test (wrong ApplicationID): 
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b011.

8: Negative test (wrong Direction): 
- TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    the Direction sub-field of the Extended NWK Frame Control field is set to 0b1.
- Th-Tool - Read out Sink Table of DUT-GPS.

9: MAC duplicate filtering test: 
- TH-GPD send 2 identical Data GPDFs formatted as in item 1 above, with the same MAC Sequence Number R >= Q+1, within 2s.
    Note: the effect can only be observed in case of commands which each command has an effect (e.g. toggle, step up). 
    We send toggle command.

10:Negative test (Auto-Commissioning and RxAfterTx both set to 0b1):
- TH-GPD send Data GPDF formatted as in item 1 above, with the following changes:
    incremented MAC Sequence number;
    AutoCommissioning field of NWK Frame Control field set to 0b1 and RxAfterTx
    field of Extended NWK Frame Control field is set to 0b1. Otherwise, the frame 
    shall be correctly formatted, incl. fresh MAC sequence number/security frame counter, as required.
- Th-Tool - Read out Sink Table of DUT-GPS.

11a:Negative test: SrcID = 0x00000000:
- TH-GPD is triggered to send a correctly protected Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    with ApplicationID = 0b000, SrcID = 0x00000000.
- Th-Tool - Read out Sink Table of DUT-GPS.

11b:Negative test: wrong SrcID: 
- TH-GPD is triggered to send a correctly protected Data GPDF formatted as in item 1 above with the following changes: 
    incremented MAC Sequence number;
    but with ApplicationID = 0b000, SrcID differing by one bit from A. 
- Th-Tool - Read out Sink Table of DUT-GPS.

12:Negative test (SecurityLevel = 0b10):
- TH-GPD send a correctly formatted GPDF, but protected with SecurityLevel = 0b10,
    security frame counter corresponding to an incremented MAC Sequence number,
    and with correctly generated MIC.
- Th-Tool - Read out Sink Table of DUT-GPS.

13:Positive test: 
- TH-GPD send a correctly formatted Data GPDF with RxAfterTx = 0b0 and updated MAC sequence number.
- Th-Tool - Read out Sink Table of DUT-GPS.

14:Basic Sink only: 
   Positive test: RxAfterTx = 0b1: 
- TH-GPD send a correctly formatted Data GPDF with RxAfterTx = 0b1 and updated MAC sequence number.
- Th-Tool - Read out Sink Table of DUT-GPS.

Pass verdict:
1:  DUT-GPS executes the GPD command. 
    DUT-GPS has Sink Table entry for SrcID = A with security frame counter value W.

2:  DUT-GPS executes the GPD command. 
    DUT-GPS has Sink Table entry for SrcID = A with GPD security frame counter value Q.

3a: 3b: 3c: 3d: 4: 5: 6: 7: 8:
    DUT-GPS does not execute the GPD command.
    DUT-GPS has Sink Table entry for SrcID = A with GPD security frame counter value Q.
    
9:  DUT-GPS executes the GPD command exactly once.

10: 11a: 11b: 12: 
    DUT-GPS does NOT execute the GPD command.
    DUT-GPS has Sink Table entry for SrcID = A with SecurityLevel = 0b00, and security frame counter value R.

13: 14:
    DUT-GPS executes the GPD command. 
    DUT-GPS has Sink Table entry for SrcID = A with updated GPD security frame counter field value.

Fail verdict:
1:  DUT-GPS does not execute the GPD command.
    OR DUT-GPS does execute the GPD command more than once.
    AND/OR DUT-GPS does not have Sink Table entry for SrcID = A with security frame counter value W.

2:  DUT-GPS does not execute the GPD command.
    OR DUT-GPS does execute the GPD command more than once.
    AND/OR DUT-GPS does not have Sink Table entry for SrcID = A with security frame counter value Q.

3a: 3b: 3c: 3d: 4: 5: 6: 7: 8:
    DUT-GPS does react.
    AND/OR DUT-GPS does not have Sink Table entry for SrcID = A with security frame counter value Q.

9:  DUT-GPS does not react on GPDF received.
    OR DUT-GPS does react more than once on GPDF received.

10: 11a: 11b: 12:
    DUT-GPS executes the GPD command.
    AND/OR DUT-GPS does not have Sink Table entry for SrcID = A 
    with security level = 0b00 and security frame counter value R.

13: 14: 
    DUT-GPS does not execute the GPD command. 
    OR DUT-GPS does execute the GPD command more than once. 
    AND/OR DUT-GPS does not have Sink Table entry for SrcID = A with updated GPD security frame counter value.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
