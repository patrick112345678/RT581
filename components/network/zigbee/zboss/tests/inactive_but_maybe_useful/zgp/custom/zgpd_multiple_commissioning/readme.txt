ZGPD Multiple commissioning test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH joins the network via commissioning.
3. TH starts commissioning again and again (TEST_COMMISSIONING_NUM number of times).

*Note, that DUT should autimatically enable commissioning with some timeout.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that TH initialized successfuly
  - "toggle_channel T commissioned T" - commissioning complete
  - "start commissioning again" - TH starts commissioning again after small timeout
  - "Test finished. Status: OK" - test completed
