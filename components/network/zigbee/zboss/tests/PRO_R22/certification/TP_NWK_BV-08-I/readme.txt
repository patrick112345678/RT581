11.8 TP/NWK/BV-08-I Buffering for Sleeping Children
Objective: DUT as router with sleeping ZED buffers at least one message.

Initial Conditions:
gZC
|
DUT ZR1
|
gZED1

gZC: PANid= 0x1AAA; Logical Address = 0x0000; 0x aa aa aa aa aa aa aa aa
DUT ZR1: PANid= 0x1AAA; 0x 00 00 00 01 00 00 00 00;
gZED: PANid= 0x1AAA; 0x 00 00 00 00 00 00 00 01;

1 Reset nodes
2 gZC starts a PAN
3 DUT ZR1 joins PAN at gZC; gZED joins PAN at DUT ZR1;


Test procedure:
1. gZC sends a counted packet message to gZED: ClusterId=0x0001, one packet, length=0x0A;
2. gZC waits (Polling Interval + 1) seconds;
3. gZC sends two messages of different length to gZED:
    ClusterId=0x0001, one packet, length=0x20
    ClusterId=0x0001, one packet, length=0x20


Pass verdict:
1) gZC sends message for gZED to DUT ZR1 with length 0x0A
2) gZED polls DUT ZR1 and receives message
3) gZC sends two messages for gZED to DUT ZR1 with length 0x20
4) gZED polls DUT ZR1 and receives either only the latest message or both messages, judged by NWK sequence number.

Fail verdict:
1) Upon first gZED poll, DUT ZR1 does not provide the message.
2) Upon second gZED poll, DUT ZR1 does not provide either message or it provides only the first message.




