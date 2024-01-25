Test described in HA test specification, clause 5.46 HA Attributes reporting

NOTE: case with checking if parameters were saved to NV RAM is not implemented

Test Harness - HA General On/Off switch as ZED
DUT          - HA General On/Off output as ZC

groups_dut starts as ZC, groups_th joins to network and sends commands for testing
reporting  functionality

commands sent by TH:

- Bind request
- Configure reporting

commands sent by DUT:
- Report x2

To start test in Linux with network simulator:
- run ./run.sh
- view .pcap file by Wireshark
- analyse test log
