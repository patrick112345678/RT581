ZGPD Battery value is 0 test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

DUT starts the network, TH joins to network and sends commands for testing

commands sent by TH:
- Attribute reporting, Power Config cluster, battery values in order: 0, 2, 0, 3
  (mix 0 values with normal voltage); default timeout for reports is 7 sec
  (TEST_REPORT_TIMEOUT), number of repors is controlled by TEST_REPORT_COUNT (test_config.h)

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that ZGPD initialized successfuly
  - "toggle_channel T commissioned T" - commissioning complete
  - "battery_notification_loop" - ZGPD sends attr reports to DUT
  - "Test finished. Status: OK" - test completed, all attr reports are sent