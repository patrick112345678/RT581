Test described in GP test specification, clause 3.1.3 Reception of non-incremental NWK sequence number

DUT ZR           - non-GP capable ZR device

3.1.3 Test procedure

1:
- Make TH request Node descriptor from the DUT-ZR in unicast, using TH's own NWK source address, and NWK sequence number N and APS Counter N.
- Make TH request Node descriptor from the DUT-ZR in unicast, using TH's own NWK source address, and NWK sequence number N-2 and APS Counter N-2.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
