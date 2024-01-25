ZGPD Unsupported Device Type test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH tries to joins to network with Undefined Device ID (0xff). DUT doesnt send Commissioining Reply.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that ZGPD initialized successfuly
  - "zgpd_send_commissioning_req" - TH sends Commissioning Req with unsupported device type
  - "Test finished. Status: OK" - test completed, test function checks that TH isnt commissioned after timeout
