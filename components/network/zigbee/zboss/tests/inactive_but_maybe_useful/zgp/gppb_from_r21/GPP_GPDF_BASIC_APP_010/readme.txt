Test described in GP test specification, clause 5.2.2.2	ApplicationID = 0b010

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.2.2.2 Test procedure
commands sent:

1:
- Make TH-GPD send a Data GPDF

2:
Negative test (IEEE address absent):
Make TH-GPD send a Data GPDF with RxAfterTx set to 0b0, with
ApplicationID = 0b010 and IEEE address of GPD absent (MAC Source
address mode 0b10 and Source address = 0xffff used instead); the
Endpoint field is present and set to X; MAC Sequence number field =
0xC4.
3:
A:
Negative test (Frame Type mismatch):
- Make TH-GPD send a Data GPDF formatted as in item 1 above with the following changes:
  MAC Sequence number field = 0xC5; the Frame Type sub-field of the NWK Frame Control field set to 0b11.
B:
Negative test (Frame Type mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xC5; the Frame Type sub-field of the NWK
Frame Control field set to 0b10.
C:
Negative test (Frame Type mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xC5; the Frame Type sub-field of the NWK
Frame Control field set to 0b01.
4:
Negative test (Zigbee Protocol Version mismatch): Make TH-GPD
send a Data GPDF formatted as in item 1 above with the following
changes: MAC Sequence number field = 0xC6; the Zigbee Protocol
Version sub-field of the NWK Frame Control field set to 0b0010.
5:
A:
Negative test (Frame Control mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xC7; the Extended NWK Frame Control field
is present and the NWK Frame Control Extension sub-field of the NWK
Frame Control field is set to 0b0.
B:
Negative test (Frame Control mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xC8; the Extended NWK Frame Control field
is NOT present and the NWK Frame Control Extension sub-field of the
NWK Frame Control field is set to 0b1.
C:
Negative test (Extended NWK Frame Control missing): Make TH-GPD
send a Data GPDF formatted as in item 1 above with the following
changes: MAC Sequence number field = 0xC8; the Extended NWK
Frame Control field is NOT present and the NWK Frame Control
Extension sub-field of the NWK Frame Control field is set to 0b0.
6:
Negative test (ApplicationID mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xC9; the ApplicationID sub-field of the
Extended NWK Frame Control field is set to 0b001.
7:
Negative test (ApplicationID mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xCA; the ApplicationID sub-field of the
Extended NWK Frame Control field is set to 0b011.
8:
Negative test (SecurityLevel mismatch): Make TH-GPD send a Data
GPDF formatted as in item 1 above with the following changes: MAC
Sequence number field = 0xCB; the SecurityLevel sub-field of the
Extended NWK Frame Control field is set to 0b10 and the KeyType subfield set to 0b1.
9:
Negative test (Direction mismatch): Make TH-GPD send a Data GPDF
formatted as in item 1 above with the following changes: MAC Sequence
number field = 0xCC; the Direction sub-field of the Extended NWK
Frame Control field is set to 0b1.
Read Proxy Table entry for TH-GPD of the DUT-GPP.
10:
Make TH-GPD send a Data GPDF with MAC sequence number 0xCD,
SecurityLevel 0b00, RxAfterTx 0b0, and GPD CommandID 0xA0 with of
the maximum allowed payload length for ApplicationID = 0b010 of 54
octets (0x00 0x01 .. 0x35).
11:
Negative test (Both Auto-Commissioning and RxAfterTx set to 0b1):
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: Auto-Commissioning sub-field of the NWK Frame
control field set to 0b1; the RxAfterTx sub-field of the Extended NWK
Frame Control field is set to 0b1; MAC sequence number = 0xCE.
12:
A:
Negative test (Wrong IEEE address):
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: IEEE address differing by 1bit.
B:
Negative test: GPD IEEE address= 0x0000000000000000:
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: IEEE address = 0x0000000000000000.
13:
Negative test (Wrong Endpoint):
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: Endpoint set to a value other that 0x00, 0xff and X
14:
Positive test (Endpoint 0x00):
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: Endpoint set to 0x00.
15:
Positive test (Endpoint 0xff):
Make TH-GPD send s Data GPDF formatted as in item 1 above with the
following changes: Endpoint set to 0xff.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
