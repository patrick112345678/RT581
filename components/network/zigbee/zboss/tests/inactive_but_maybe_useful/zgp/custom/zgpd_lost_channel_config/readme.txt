ZGPD Lost channel config test

Test Harness - ZGPD Door sensor device
DUT          - Simple Coordinator (/devices folder)

1. DUT starts the network.
2. TH tries to joins to network (send Channel Requests).
3. DUT sends Channel Config packets, but TH skips them (LOST_CHANNEL_CONFIG_PACKETS_NUM number of times).
4. After it TH receives Channel Config and process commissioning.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log:
  - "ZGPD Device STARTED OK" indicates that TH initialized successfuly
  - "lost ch config packet" - shows every time when TH losts Channel Config
  - "toggle_channel T commissioned T" - commissioning complete
  - "Test finished. Status: OK" - test completed, TH successfully commissinoined after lost Channel Configs
