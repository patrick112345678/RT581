Test described in GP test specification, clause 4.2.2 Basics and 4.2.2.2 ApplicationID = 0b010

Test Harness tool - as ZR
Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network

TH-GPD uses ApplicationID = 0b010 and GPD IEEE address A; Endpoint X and Y.
A pairing is established between the TH-GPD (IEEE address and endpoint X) and DUT-GPS
(e.g. as a result of any of the commissioning procedures), with security level = 0b00,
TH-GPD uses incremental MAC sequence number; the initial value N is known.

4.2.2.2 Test procedure
commands sent by TH-tool and TH-gpd:

1:
A:
- Make TH-GPD send a Data GPDF with:
    - MAC source address field carrying GPD IEEE address A;
    - NWK Frame Control field with the following values:
        - the Frame type sub-field set to 0b00,
        - the Zigbee Protocol Version sub-field set to 0b0011.
        - the NWK Frame Control Extension sub-field set to 0b1;
    - the Extended NWK Frame Control field is present, with the following values:
        - the ApplicationID sub-field set to 0b010,
        - the Direction sub-field set to 0b0.
    - SrcID field absent;
    -  Endpoint field present and carrying value X.
    - CommandID as supported by the DUT-GPS.
- Read Sink Table entry of DUT-GPS.

B:
- Make TH-GPD send a Data GPDF, formatted as in 1A above, but with Endpoint field present and carrying value 0x00.
- Read Sink Table entry of DUT-GPS.

C:
- Make TH-GPD send a Data GPDF, formatted as in 1A above, but with Endpoint field present and carrying value 0xff.
- Read Sink Table entry of DUT-GPS.

2: Negative test: wrong GPD endpoint (Y):
- Make TH-GPD send a Data GPDF as specified in 1, only with Endpoint field carrying the value Y (from the range 0x01-0xef).
- Read Sink Table entry of DUT-GPS.

3: Negative test (IEEE address absent):
- Make TH-GPD send a Data GPDF formatted as specified in 1 above, with RxAfterTx set to 0b0, with ApplicationID = 0b010
  and IEEE address of GPD absent (MAC Source address mode 0b10 and Source address = 0xffff used instead); the Endpoint field is present and set to X.
- Read out Sink Table of DUT-GPS. If possible, check command execution by DUT-GPS.

4: Negative test: GPD IEEE address= 0x0000000000000000:
- TH-GPD (or TH-Tool in the role of TH-GPD) is triggered to send a correctly formatted Data GPDF, but with ApplicationID = 0b010, GPD IEEE address = 0x0000000000000000.

5: Positive test:
- Make TH-GPD send a correctly formatted Data GPDF with updated MAC sequence number.
- Read Sink Table of DUT-GPS.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
