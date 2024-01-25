ZGPD Send cmd before commissioining test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH sends Attribute reports (TEST_NOTIFICATIONS_NUM number of times) before join to network.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that ZGPD initialized successfuly
  - "battery_notification_loop" - ZGPD sends attr reports to DUT
  - "Test finished. Status: OK" - test completed
