Test described in GP test specification, clause 5.2.5.3	SecurityLevel = 0b11, ApplicationID = 0b000

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.2.5.3 Test procedure
commands sent:

1:
- TH-GPD sends a Data GPDF, with the Extended NWK Frame Control field present, the ApplicationID sub-field set to 0b000,
  the SecurityKeyType 0b0, the SecurityLevel sub-field set to 0b11, the SecurityFrameCounter field present and having
  the correct value N; the 4B SrcID field present and set to Z, the Endpoint field absent, the GPD command payload
  encrypted and authenticated with 4B MIC.
- Read out Proxy Table of DUT-GPP.


2: Negative test (wrong key):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present,
  the SecurityKeyType 0b0, the SecurityLevel sub-field set to 0b11, the SecurityFrameCounter field present and having
  the correct value M >= N+1; the payload protected with a wrong key.

3: Negative test (wrong key type):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present,
  the SecurityKeyType 0b1, the SecurityLevel sub-field set to 0b11, the SecurityFrameCounter field present and having
  the correct value P >= M+1; the payload correctly protected.

4: Negative test (unprotected frame):
- TH-GPD sends an unprotected Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control
  field present and the SecurityLevel sub-field set to 0b11.

5: Negative test (lower security level):
- TH-GPD sends a Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present,
  the SecurityKeyType 0b1, the SecurityLevel sub-field incorrectly set to 0b10, the SecurityFrameCounter field present
  and having the correct value Q >= P+1; the payload protected.

6: Negative test (replayed frame counter):
- Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present, the SecurityKeyType 0b0,
  the SecurityLevel sub-field set to 0b11, the SecurityFrameCounter field present and having an old value N-1;
  the payload encrypted and authenticated with 4B MIC.

7: Negative test (wrong direction):
- Data GPDF with ApplicationID = 0b000, SrcID = Z, with the Extended NWK Frame Control field present, the SecurityKeyType 0b0,
  the SecurityLevel sub-field set to 0b11, the SecurityFrameCounter field present and having correct value N+1;
  the payload encrypted and authenticated with 4B MIC, but with the Direction sub-field of the Extended NWK Frame Control field set to 0b1.
- Read out Proxy Table of DUT-GPP.

8:
- TH-GPD sends a correctly protected Data GPDF with ApplicationID = 0b000, SrcID = Z, with SecurityFrameCounter value N+1
- Read out Proxy Table of DUT-GPP.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
