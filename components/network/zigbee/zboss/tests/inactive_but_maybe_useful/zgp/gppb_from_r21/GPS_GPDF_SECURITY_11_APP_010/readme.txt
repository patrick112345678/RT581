29.2.3.5 SecurityLevel = 0b11 and ApplicationID = 0b010
Note: negative tests for GPDF frame format for ApplicationID = 0b010 are done in
sec. 29.2.2.2; negative tests for security level 0b11 are done in sec. 29.2.3.3.
Completing those tests is pre-requisite for running the current test.
Thus, this test contains only one positive test.

Test Harness tool - as ZR
Test Harnes GPD   - as ZGPD
DUT GPS           - GPS as ZC

dut_gps starts as ZC, th-tool joins to network


TH-GPD and DUT-GPS have a pairing with ApplicationID = 0b010,
GPD IEEE address = A, Endpoint X, SecurityLevel = 0b11 and any shared key type
(gpSharedSecurityKeyType 0b001, 0b010 or 0b011); the key value
(gpSharedSecurityKey) and SecurityFrameCounter value are known.

4.2.3.3 Test procedure
commands sent by TH-tool and TH-gpd:

1:
TH-GPD sends Data GPDF, with the Extended NWK Frame Control field present,
ApplicationID = 0b010, GPD IEEE address = A, Endpoint X, the SecurityLevel
sub-field set to 0b11, SecurityKey set to 0b0, the SecurityFrameCounter field
present and set to correct value N, and the payload correctly authenticated
with 4B MIC.
Read out Sink Table of DUT-GPS.

Pass verdict:
1:  DUT-GPS successfully security processes the received GPDF, stores the new
frame counter value in its Sink Table entry for the GPD ID =A and executes the
GPD command.


Fail verdict:
1:  DUT-GPS does not execute the GPD command.
    AND/OR DUT-GPS does not store the new frame counter value in its Sink Table
    entry for the GPD ID =A.


To start test in Linux with network simulator:
- run ./runng.sh
- view .pcap file by Wireshark
- analyse test log
