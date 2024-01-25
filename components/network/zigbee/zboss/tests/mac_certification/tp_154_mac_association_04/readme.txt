
Test TP/154/MAC/ASSOCIATION-03


Check D.U.T. (as device) correctly generates association request frame and receives association
response frame with positive response (short address)

To pass test Zigbee stack should be build with defined ZB_MAC_TESTING_MODE

TH as coordinator shall set up a non‐beacon enabled PAN.
DUT transmits MAC association request command frame .
DUT will transmit MAC data request command frame to coordinator.
Association response MAC command frame issued from the TH.
DUT will transmit Ack frame in response to association response MAC 
command frame from TH.

The DUT shall return a Success result, such that: 
DUT returns MAC primitive (check in xxx.log file):

The DUT shall return a Success result, such that:
DUT returns MAC primitive
MLME-ASSOCIATE.confirm (
ShortAddress= 0x3344,
Status = 0x00 = SUCCESS,
SecurityLevel = 0x00,
KeyIdMode = xx,
KeySource = xx,
KeyIndex = xx
)
Fail:
DUT does not send MAC data request command frame
DUT does not send ACK in response to Association response frame
DUT does not return MLME-ASSOCIATE.confirm primitive
ShortAddress is incorrect.
Status is not SUCCESS

