Test described in GP test specification, clause 2.4.3.8 DUT-GPD with OOB key encrypted (shared key requested, not delivered)

DUT GPD           - GPD ON/OFF device

2.4.3.8 Test procedure
commands sent by DUT:

1:
- TH-GPS enters commissioning mode with gpsCommissioningExitMode = On first Pairing success.
  Commissioning action is (repeatedly, if required) performed on DUT-GPD (if manually: not more often than once per second),
  until TH-GPS provides commissioning success feedback.
  After successful completion of GPD Channel Request/GPD Channel Configuration command,
  DUT-GPD sends two correctly formatted  Commissioning GPDF (with KeyRequest = 0b1 and encrypted OOB key present).
  Upon reception of a second GPD Commissioning command, TH-GPS sends Commissioning Reply GPDF with:
    - secured as the triggering Commissioning GPDF;
    - sub-fields of the Options field set to:
        - GPDsecurityKeyPresent=0b0;
        - GPDkeyEncryption = 0b0, KeyType = 0b100,
    - GPDsecurityKey field, GPD Key MIC field and  Frame Counter field absent.

2:
- After a couple of seconds, the DUT-GPD is triggered to send Data GPDF.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
