Test described in GP test specification, clause 5.4.1.4	CT-based commissioning, OOB key

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool joins to network, th-gpd pairing with th-tool

5.4.1.4 Test procedure
commands sent:

1:
 - TH-Tool sends GP Pairing for GPD ID = N with SecurityKeyType = 0b100, SecurityLevel >= 0b10 and
   SecurityKey [c0 : c1 : c2 : c3 : c4 : c5 : c6 : c7 : c8 : c9 : ca : cb : cc : cd : ce : cf].
 - Read out Proxy Table of the DUT-GPP.

2: Negative test: SecurityLevel = 0b01:
 - Clean Proxy Table.
 - TH-Tool sends GP Pairing for GPD ID = N with SecurityLevel = 0b01.
 - Read out Proxy Table of the DUT-GPP.

3: Negative test: SrcID = 0x00000000:
 - TH-Tool sends GP Pairing for ApplicationID = 0b000, SrcID = 0x00000000.
 - Read out Proxy Table of the DUT-GPP.

4: Negative test: GPD IEEE address = 0x0000000000000000:
 - TH-Tool sends GP Pairing for ApplicationID = 0b010, GPD IEEE address = 0x0000000000000000 and Endpoint X.
 - Read out Proxy Table of the DUT-GPP.

5: Positive test: SrcID = 0xFFFFFFFF:
 - TH-Tool sends GP Pairing for ApplicationID = 0b000, SrcID = 0xFFFFFFFF, SecurityLevel - 0b00.
 - Read out Proxy Table of the DUT-GPP.
 - TH-Tool sends unprotected Data GPDF with SrcID = 0x12345678.
 - TH-Tool sends unprotected Data GPDF with SrcID = 0x87654321.

6: Positive test: GPD IEEE address = 0xFFFFFFFFFFFFFFFF:
 - Clean Proxy Table.
 - TH-Tool sends GP Pairing for ApplicationID = 0b010, GPD IEEE address = 0xFFFFFFFFFFFFFFFF and Endpoint X, SecurityLevel - 0b00.
 - Read out Proxy Table of the DUT-GPP.
 - TH-Tool sends unprotected Data GPDF with IEEE = 0x1122334455667788.
 - TH-Tool sends unprotected Data GPDF with IEEE = 0x8877665544332211.

7: Positive test: GPD IEEE address = X, Endpoint 0xff:
 - Clean Proxy Table.
 - TH-Tool sends GP Pairing for ApplicationID = 0b010, GPD IEEE address = 0xFFFFFFFFFFFFFFFF and Endpoint X, SecurityLevel - 0b00.
 - Read out Proxy Table of the DUT-GPP.
 - TH-Tool sends unprotected Data GPDF with IEEE = X, Endpoint = E.
 - TH-Tool sends unprotected Data GPDF with IEEE = X, Endpoint = F.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
