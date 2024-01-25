Test described in GP test specification, clause 5.2.5.2	SecurityLevel = 0b10, ApplicationID = 0b000

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.2.5.2 Test procedure
commands sent:

1:
- TH-GPD sends Data GPDF, with the Extended NWK Frame Control field present, the ApplicationID sub-field set to 0b000,
  the SecurityLevel sub-field set to 0b10, SecurityKey set to 0b0, the SecurityFrameCounter field present and set to N,
  the 4B SrcID field present and set to Z, the Endpoint field absent, and the payload correctly authenticated with 4B MIC.
- Read out Proxy Table of DUT-GPP.

2: Negative test (wrong key):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = Z, protected with a correct security level (0b10),
  correct frame counter value (=M>=N+1), correct key type (0b0) but wrong key.

3: Negative test (wrong key type):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = Z, protected with a correct security level (0b10),
  correct SecurityFrameCounter value (=P>=M+1), correct key but wrong key type (0b1).

4: Negative test (unprotected frame):
- TH-GPD sends an unprotected Data GPDF, with ApplicationID = 0b000, SrcID = Z, and with  MAC Sequence number value (=Q>=P+1). (SecurityLevel = 0b00).

5: Negative test (lower security level):
- TH-GPD sends a Data GPDF,  with ApplicationID = 0b000, SrcID = Z, with security level lower than the before (SecurityLevel = 0b01), with counter value >=Q+1.

6: Negative test (replayed frame):
- TH-GPD sends a correctly secured replayed Data GPDF, with ApplicationID = 0b000, SrcID = Z  and with old SecurityFrameCounter N-1.

7: Negative test (wrong direction):
- TH-GPD sends Data GPDF, with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present,
  the SecurityKeyType 0b0, the SecurityLevel sub-field set to 0b10, the SecurityFrameCounter field present and having
  a correct value N+1; the payload authenticated with 4B MIC, but with the Direction sub-field of the Extended NWK Frame Control field set to 0b1.
- Read out Proxy Table of DUT-GPP.

8:
- TH-GPD sends a correctly protected Data GPDF with ApplicationID = 0b000, SrcID = Z, with SecurityFrameCounter value N+1.
- Read out Proxy Table of DUT-GPP.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
