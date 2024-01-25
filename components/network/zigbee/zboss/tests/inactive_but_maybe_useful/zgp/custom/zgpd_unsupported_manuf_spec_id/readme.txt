ZGPD Unsupported Manufacturer Specific ID test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH tries to joins to network with Unsupporter Manufacturer Specific ID (0xffff). DUT doesnt send Commissioining Reply.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that ZGPD initialized successfuly
  - "zgpd_send_commissioning_req" - TH sends Commissioning Req with unsupported manufacturer specific id
  - "Test finished. Status: OK" - test completed, test function checks that TH isnt commissioned after timeout
