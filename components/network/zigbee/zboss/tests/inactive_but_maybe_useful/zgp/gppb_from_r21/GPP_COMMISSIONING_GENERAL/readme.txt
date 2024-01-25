Test described in GP test specification, clause 5.4.1.13 General commissioning tests

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-gps joins to network, th-gpd pairing with th-gps

5.4.1.13 Test procedure
commands sent:

1. Negative test: Different sequence number/frame counter in both modes:
- TH-GPS sends GP Proxy Commissioning Mode (Action=Enter).
  Commissioning is successfully performed between TH-GPD and TH-GPS via DUT-GPP; gpdSecurityLevel >= 0b10 is used.
  After TH-GPS sends GP Pairing, read out Proxy Table entry of the DUT-GPP.
- TH-GPD is made to send a Data GPDF with wrong security frame counter value, i.e. lower than the last value used in commissioning mode.
- Read out Proxy Table entry of the DUT-GPP.

2. Maximum  length of GPD Commissioning:
A:
- Clear Proxy Table of DUT-GPP.
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter).
- TH-GPD using ApplicationID = 0b000 sends GPD Commissioning command, with payload having the maximum length for ApplicationID = 0b000: 55 octets.
B:
- Clear Proxy Table of DUT-GPP.
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter).
- TH-GPD using ApplicationID = 0b010 sends GPD Commissioning command, with payload having the maximum length for ApplicationID = 0b010: 50 octets.

3. Malformed commissioning GPDF:
A. Negative test: Commissioning GPDF with Auto-Commissioning sub-field set:
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter, Unicast communication = 0b0).
- TH-GPD sends Commissioning GPDF, with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with Auto-Commissioning sub-field of the NWK Frame Control field set to 0b1.
B. Negative test Commissioning GPDF with wrong FrameType:
- TH-GPD sends Commissioning GPDF, with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with FrameType sub-field of the NWK Frame Control field set to 0b11.
C. Negative test: Commissioning GPDF with wrong ZigbeeProtocolVersion:
- TH-GPD sends Commissioning GPDF, with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with ZigbeeProtocolVersion sub-field of the NWK Frame Control field set to 0b0010.
D. Negative test: Commissioning GPDF with wrong ApplicationID:
- TH-GPD sends Commissioning GPDF, with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with ApplicationID sub-field of the Extended NWK Frame Control field set to 0b011.
E. Negative test: Commissioning GPDF with wrong Direction:
- TH-GPD sends Commissioning GPDF, with all the options as supported by the TH-GPS, with RxAfterTx = 0b0 (or not present),
  but with Direction sub-field of the Extended NWK Frame Control field set to 0b1.

4. GPD ID = 0x00..00:
A. Negative test: SrcID = 0x00000000: 
- Clear Proxy Table of DUT-GPP.
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter).
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send the Commissioning GPDF with: RxAfterTx = 0b0 and ApplicationID = 0b000, SrcID = 0x00000000.
B. Negative test: GPD IEEE address = 0x0000000000000000:
- Clear Proxy Table of DUT-GPP.
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter).
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send the Commissioning GPDF with: RxAfterTx = 0b0 and ApplicationID = 0b010,
  GPD IEEE address (in MAC header source address field) = 0x0000000000000000 and Endpoint X.

5. Extendibility of GPD Commissioning frame:
- Clear Proxy Table of DUT-GPP.
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter).
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send the Commissioning GPDF with:
   - Auto-Commissioning sub-field of the NWK Frame Control field set to 0b0,
   - the Extended NWK Frame Control field present and its RxAfterTx sub-field set to 0b0,
   - the Reserved sub-fields of the Options field set to 0b1
   - all other sub-fields and fields set correctly;
   - at the end of the Commissioning GPDF (i.e. after ApplicationInformation, if included), 5 additional bytes of payload, 0xe0 0xe1 0xe2 0xe3 0xe4, are applied;
     the maximal GPDF length is NOT exceeded.

6. Negative test (gpdSecurityLevel = 0b01):
- Clear Proxy Table of DUT-GPP.
- TH-Tool sends GP Pairing command with all settings as supported by DUT-GPP, but with SecurityLevel sub-field of the Options field set to 0b01.
- TH-ZR reads the Proxy Table attribute of DUT-GPP, using ZCL Read Attributes command.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
