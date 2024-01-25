
Test TP/154/MAC/ASSOCIATION-01

Check D.U.T. (as coordinator) correctly receives association request frame and generates positive
response

To pass test Zigbee stack should be build with defined ZB_MAC_TESTING_MODE

DUT as coordinator shall set up a non‐beacon enabled PAN.
Tester transmits MAC association request command frame .
Tester will transmit MAC data request command frame to coordinator, DUT
Association response MAC command frame issued from the DUT.
Tester will transmit Ack frame in response to association response MAC 
command frame from DUT.

The DUT shall return a Success result, such that: 
DUT returns MAC primitive (check in xxx.log file):

MLME‐COMM‐STATUS.indication( 
PANid = 0x1aaa, 
SrcAddrMode = 0x03 = ExtendedAddress, 
SrcAddr = 0xACDE480000000001, 
DstAddrMode = 0x03= ExtendedAddress, 
DstAddr = 0xACDE480000000002, 
Status = 0x00 = SUCCESS, 
SecurityLevel = 0x00, 
KeyIdMode = xx, 
KeySource = xx, 
KeyIndex = xx 
)

Fail:  
DUT does not send ACK in response to MAC data request command frame 
ACK does not have Frame pending field set 
DUT does not send association response frame 
DUT does not return MLME‐COMM‐STATUS.indication primitive 
Status is not SUCCESS 

