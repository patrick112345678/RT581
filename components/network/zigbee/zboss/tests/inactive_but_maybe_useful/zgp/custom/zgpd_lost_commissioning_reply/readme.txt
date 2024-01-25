ZGPD Lost Commissioining Relpy test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH tries to joins to network (send Channel Requests).
3. DUT sends Channel Config packets, TH receives them. TH sends Commissioning packets.
4. DUT sends Commissioning Reply packets, but TH skips them (LOST_COMM_REPLY_PACKETS_NUM number of times).
5. After it TH receives Commissioning Reply and process commissioning.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that TH initialized successfuly
  - "lost comm reply packet" - shows every time when TH losts Channel Config
  - "toggle_channel T commissioned T" - commissioning complete
  - "Test finished. Status: OK" - test completed, TH successfully commissinoined after lost Commissioning Reply
