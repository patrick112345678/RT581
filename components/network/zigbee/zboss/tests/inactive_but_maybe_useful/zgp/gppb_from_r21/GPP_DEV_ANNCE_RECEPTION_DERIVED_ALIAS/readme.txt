Test described in GP test specification, clause 3.1.2 Device_annce reception

DUT ZR           - non-GP capable ZR device

3.1.2 Test procedure

1:
- Make TH-Tool/TH-GPS send Device_annce with Alias_short_addr WXYZ (conflicting with short address of DUT-ZR) and IEEE address 0xFFFFFFFFFFFFFFFF.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
