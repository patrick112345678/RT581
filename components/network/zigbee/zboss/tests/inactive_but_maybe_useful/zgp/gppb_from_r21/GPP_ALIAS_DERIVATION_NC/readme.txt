Test described in GP test specification, clause 5.3.3.2 Alias derivation. Subcase: No alias conflict

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-gps joins to network, th-gpd pairing with th-gps

5.3.3.2.2 Test procedure
commands sent:

1:
- TH-GPD has SrcID 0x12345678.
- Set DUT-GPP in commissioning mode by sending GP Proxy
  Commissioning Mode (Action=Enter).
- Make TH-GPD send the Commissioning GPDF.

5.3.3.2.3 Expected Outcome

Pass verdict:
-  DUT-GPP sends a GP Commissioning Notification with NWK SrcAddr equal to the alias_srcAddr 0x5678 as and
NWK sequence number equal to MAC header Sequence number field of the triggering GPDF � 12 mod 256.

Fail verdict:
- The DUT-GPP does not send a GP Commissioning Notification, formatted as specified above.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
