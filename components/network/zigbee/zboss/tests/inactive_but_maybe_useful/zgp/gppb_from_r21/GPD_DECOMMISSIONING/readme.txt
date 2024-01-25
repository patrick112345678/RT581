Test described in GP test specification, clause 2.4.5 GPD Decommissioning command

TH GPS            - GPS as ZC
DUT GPD           - GPD ON/OFF device

th_gps starts as ZC and enter in commissioning mode with exit mode:
gpsCommissioningExitMode = On first Pairing success

2.4.5 Test procedure
commands sent by DUT:

1:
- Read out Sink Table of the TH-GPS.
- DUT-GPD is made to send a Data GPDF.
- Put TH-GPS in commissioning mode.
- DUT-GPD is made to send Decommissioning GPDF.
- If ApplicationID = 0b010, and if supported by the DUT-GPD, the Decommissioning GPDF carries the particular paired Endpoint X;
  otherwise, if supported by the DUT-GPD, Decommissioning GPDF carries the particular paired Endpoint 0xff.
- Read out Sink Table of the TH-GPS.

Note: DUT-GPD supports multiple Decommissioning frames (with different Endpoint or SrcID values),
      the test has to be repeated for each combination; the existing pairing should match the data in the Decommissioning GPDF.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log

To start test on cc2538 platform:
1. Run TH-GPS
2. Run TH-Tool
3. Run DUT-GPD
4. Press left button at TH-GPS (th-gps will entering in commissioning mode)
5. Press left button at DUT-GPD (dut-gps send commissioning GPDF with endpoint 1)
6. Press left button at TH-GPS (th-gps will entering in commissioning mode)
7. Press left button at DUT-GPD (dut-gps send commissioning GPDF with endpoint 2)
8. Press left button at TH-Tool (th-tool read sink table)
9. Press left button at DUT-GPD (dut-gpd send toggle GPDF with endpoint 1)
10. Press left button at DUT-GPD (dut-gpd send toggle GPDF woth endpoint 2)
11. Press left button at TH-GPS (th-gps will entering in commissioning mode)
12. Press left button at DUT-GPD (dut-gpd send decommissioning GPDF with endpoint 1)
13. Press left button at TH-Tool (th-tool read sink table)
14. Press left button at DUT-GPD (dut-gpd send decommissioning GPDF with endpoint 2)
15. Press left button at TH-Tool (th-tool read sink table)
