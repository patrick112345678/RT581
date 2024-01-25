Test described in GP test specification, clause 5.2.2.1	ApplicationID = 0b000

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.2.2.1 Test procedure
commands sent:

1:
- Make TH-GPD send a Data GPDF

2:
- Make TH-GPD send a Data GPDF as in item 1 above with the changes

3:
A:
Negative test (Frame Type mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC5; the Frame Type sub-field of the NWK Frame Control field set to 0b11.
B:
Negative test (Frame Type mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC5; the Frame Type sub-field of the NWK Frame Control field set to 0b10.
C:
Negative test (Frame Type mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC5; the Frame Type sub-field of the NWK Frame Control field set to 0b01.
D:
Negative test (Zigbee Protocol Version mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC5; the Zigbee Protocol Version sub-field of the NWK Frame Control field set to 0b0010.

4:
Negative test (Frame Control mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC6; the Extended NWK Frame Control field is present and the NWK Frame Control Extension sub-field of the NWK Frame Control field is set to 0b0.

5:
Negative test (Frame Control mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC7; the Extended NWK Frame Control field is NOT present and the NWK Frame Control Extension sub-field of the NWK Frame Control field is set to 0b1.

6:
Negative test (ApplicationID mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC8; the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b001.

7:
A:
Negative test (ApplicationID mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC9; the ApplicationID sub-field of the Extended NWK Frame Control field is set to 0b011.
B:
Negative test (SecurityLevel mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC9; the SecurityLevel sub-field of the Extended NWK Frame Control field is set to 0b10 and the KeyType sub-field set to 0b1.

8:
Negative test (Direction mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xCA; the Direction sub-field of the Extended NWK Frame Control field is set to 0b1.
  Read Proxy Table entry for TH-GPD of the DUT-GPP.

9:
- Make TH-GPD send 2 identical Data GPDFs formatted as in item 1 above, with the same MAC Sequence Number, 0xCB, within 2s

10:
- Make TH-GPD send a Data GPDF with MAC sequence number 0xCC, SecurityLevel 0b00, RxAfterTx 0b0, and GPD CommandID 0xA0 with of the max allowed payload size for ApplicationID = 0b000 of 59 octets (0x00 0x01 .. 0x3A).

11:
Negative test (Both Auto-Commissioning and RxAfterTx set to 0b1):
- Make TH-GPD send s Data GPDF formatted as in item 1 above with the following changes:
  Auto-Commissioning sub-field of the NWK Frame control field set to 0b1; the RxAfterTx sub-field of the Extended NWK Frame Control field is set to 0b1.

12:
A:
Negative test (Wrong SrcID):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: SrcID differing by 1bit.

B:
Negative test (Wrong SrcID = 0x00000000):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes: SrcID = 0x00000000.

13:
Positive test:
- Make TH-GPD send a correctly formatted Data GPDF as in item 1 above, only with updated MAC sequence number.

14:
Basic Proxy only:
Positive test:
RxAfterTx = 0b1:
- Make TH-GPD send a correctly formatted Data GPDF as in item 1 above, only with RxAfterTx = 0b1 and updated MAC sequence number.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
