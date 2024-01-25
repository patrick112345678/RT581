TP/PRO/BV-14 Address Conflict to ZC
Verify ZC ignores address conflict for its short address

DUT ZC
  |
 gZR

Test procedure
1. Initiate broadcast address with Conflict error packet (short address of DUT ZC ie. 0x0000)
over the air by some means.(Smarter RF tool of Chipcon can used to generate this packet)

Pass verdict:
1 DUT ZC does not send address conflict error over the air;
2 DUT ZC does not change its short address;

Fail verdict:
1. DUT ZC sends address conflict error over the air;
2. DUT ZC changes its short address;
