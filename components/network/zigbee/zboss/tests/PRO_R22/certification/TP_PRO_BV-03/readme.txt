TP/PRO/BV-03 Rejoin end device (Rx – Off)
Validate end device (with Rx on idle=false/true) does not change its short address
on rejoin, when it leaves from one parent and joins with another parent.

DUT ZC
DUT ZR
gZED

DUT ZR joins DUT ZC.g ZED joins PAN at DUT ZR.

Test procedure:
1. Hard reset the DUT ZR;

Pass verdict:
1. After DUT ZR reset, gZED attempts to transmit data request packet
to DUT ZR; upon failure (or based on “No Ack”), gZED initiates rejoin
procedure by performing an active scan.
2. DUT ZC transmits a beacon indicating available device capacity
3. gZED unicasts a Rejoin Request command to DUT ZC
4. DUT ZC issues a rejoin response to gZED
5. gZED rejoins the network.
6. DUT ZC assigns the same short address to gZED. (ZED does not
change addresses on rejoin). gZED issues Device Announce in its
new location but with same short address.
7. gZED polls DUT ZC and receives acknowledgement with interval as
defined by stack profile (say 7.5 seconds).

Fail verdict:
1. After DUT ZR reset, gZED does not attempt to transmit data request
packet to DUT ZR or upon failure polling, gZED does not initiate
rejoin procedure and does not perform an active scan.
2. DUT ZC does not transmit a beacon indicating available device
capacity
3. gZED does not unicast a Rejoin Request command to DUT ZC
4. DUT ZC does not issues a rejoin response to gZED with same short
address
5. gZED does not rejoin the network or it does not issue a device
announcement with old short address.
6. gZED does not poll DUT ZC.

