Test described in GP test specification, clause 2.4.3.3 Test Security commissioning

Test Harness GPS  - as ZC (GPT+/GPC)
DUT GPD           - GPD

--PANId = Generated in a random manner (within the range 0x0001 to 0x3FFF)
--Channel = as supported by the DUT-GPD
--Logical Address = Generated in a random manner (within the range 0x0000 to 0xFFF7)
--IEEE address = manufacturer-specific (defined in test_config.h)
--GPEP supports the application functionality of the DUT-GPD (see Section A.4.3 of [R3]).
--GPD ID = manufacturer-specific (defined in test_config.h)
--DUT-GPD  capable  of  sending  Commissioning GPDF.

No pairing exists between TH-GPS and DUT-GPD

Configuration:
TH-GPS is configured with:
    gpsSecurityLevel >= 0b10;
    gpSharedSecurityKeyType = 0b011 (NWK key derived GPD group key); 
	the value of the Zigbee NWK key shall be chose such that the 
	derived GPD key is different than the value of the OOB key of DUTGPD;
    default gpLinkKey (‘ZigbeeAlliance09’);

DUT-GPD is configured with:
    gpdSecurityLevel >=0b10
    DUT-GPD has OOB key.

Test procedure:

  TH-GPS starts as ZC and enters commissioning mode with gpsCommissioningExitMode = On First Pairing Success.
    Commissioning action is (repeatedly, if required) performed on DUT-GPD 
    (if manually: not more often than once per second), until TH-GPS provides commissioning success feedback.
    After successful completion of GPD Channel Request/GPD Channel Configuration command, 
    upon receipt of a second correctly formatted Commissioning GPDF 
    (with KeyRequest = 0b1 and encrypted OOB key present), 
    TH-GPS sends correctly formatted Commissioning Reply GPDF with:
      secured as the triggering Commissioning GPDF;
      sub-fields of the Options field set to: 
	GPDsecurityKeyPresent=0b1; 
        GPDkeyEncryption = 0b1, 
        KeyType = 0b011, 
        GPDsecurityKey field present and carrying the GP group key derived from the NWK key, encrypted, 
        GPD Key MIC field is present, 
        Frame Counter field present.
    After commissioning completes, DUT-GPD is triggered to send Data GPDF.

Pass Verdict:
    1) DUT-GPD sends Data GPDF protected, with Extended NWK Frame Control field present and its SecurityKey subfield set to 0b0.

Fail Verdict:
    1) DUT-GPD does not sent Data GPDF, formatted exactly as specified in item 1 above.

To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
