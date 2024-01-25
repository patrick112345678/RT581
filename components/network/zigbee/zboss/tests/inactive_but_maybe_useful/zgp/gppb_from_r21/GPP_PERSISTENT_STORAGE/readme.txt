Test described in GP test specification, clause 5.1.4 Persistent storage of Proxy Table

Test Harness tool - as ZR
DUT GPP           - GPPB as ZC
Test Harness GPS  - as ZR
Test Harness GPD  - as ZGPD

dut_gpp starts as ZC, th-tool, th-gps joins to network, th-gpd pairing with th-gps

5.1.4 Test procedure
commands sent:

1:
A:
- TH-tool Read out Proxy Table of DUT-GPP.
B:
- A pairing with derived groupcast communication mode and SecurityLevel >= 0b10 is established
  between the TH-GPD and TH-GPT (e.g. as a result of any of the commissioning procedures); Security frame counter has an initial value M.
  The corresponding information is also included in DUT-GPP (e.g. as a result of any of the commissioning procedures);
  Security frame counter has an initial value N.
- TH-tool Read out Proxy Table of DUT-GPP.
C:
- Make TH-GPD send a correctly formatted and protected Data GPDF with incremented Security frame counter N => M+1.
- TH-tool Read out Proxy Table of DUT-GPP.

2:
A:
- Switch off the DUT-GPP for approximately 5 seconds.
- Switch the DUT-GPP back on and make sure it can communicate on the network.
TH-tool Read out Proxy Table of DUT-GPP.
B:
Make TH-GPD send a correctly formatted and protected Data GPDF with incremented Security frame counter O => N+1.
TH-tool Read out Proxy Table of DUT-GPP.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
