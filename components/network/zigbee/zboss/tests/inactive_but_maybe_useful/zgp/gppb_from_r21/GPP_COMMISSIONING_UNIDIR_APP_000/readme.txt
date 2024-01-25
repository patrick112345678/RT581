Test described in GP test specification, clause 5.4.1.8 Unidirectional commissioning, ApplicationID = 0b000

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-gps joins to network, th-gpd pairing with th-gps

5.4.1.8 Test procedure
commands sent:

1:
- TH-GPS1 sends GP Proxy Commissioning Mode (Action = Enter, Unicast communication = 0b0).
  (E.g. as a result of commissioning action; if performed manually: not more often than once per second),
  TH-GPD sends GPD Commissioning command (GPD CommandID = 0xE0), at least on one channel being the operational channel of the network, with:
   - MAC sequence number A;
   - Auto-Commissioning sub-field of the NWK Frame Control field set to 0b0;
   - RxAfterTx sub-field of the Extended NWK Frame Control field set to 0b0 (or absent);
   - GPD command payload:
      - sub-fields of the Options field set to: GP Security Key request = 0b0, Extended Options field = 0b1
      - sub-fields of the Extended Options field set to: SecurityLevel capabilities >= 0b10, KeyType = 0b100, GPD Key present = 0b1, GPD outgoing counter present = 0b1.
      - GPD Key field present and carrying OOB key of DUT-GPD.
      - GPD Key MIC field present, if GPD Key encryption sub-field of the Extended Options field was set to 0b1;
      - GPD outgoing counter field present and carrying the value N.

2:
- TH-GPS provides user success feedback and sends GP Pairing.
- Read out Proxy Table entry of the DUT-GPP.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
