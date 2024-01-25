
Test TP/154/MAC/SCANNING-04

Perform an active scan on several channels to check positive response when coordinator(s) present 
(short address without beacon payload)

To pass test Zigbee stack should be build with defined ZB_MAC_TESTING_MODE
Test sets mac_auto_request = 1 to store all beacons, all received beacons are reported
into log file in zb_mlme_scan_confirm(). If mac_auto_request = 0, beacons are analysed
in zb_mlme_beacon_notify_indication() on every beacon frame receive.

On test finish check *.log, pan descriptor should agree with one described in
the test spec:

 
Pass:
It is possible to verify by a PAN analyzer that a beacon request command frame 
was issued from the DUT. 
    Frame Length: 10 bytes 
    IEEE 802.15.4 
    Frame Control: 0x0803 
    .... .... .... .011  = Frame Type: Command (0x0003) 
    .... .... .... 0...  = Security Enabled: Disabled 
    .... .... ...0 ....  = Frame Pending: No more data 
    .... .... ..0. ....  = Acknowledgment Request: Ack not required 
    .... .... .0.. ....  = Intra-PAN: Not within the PAN 
    .... ..00 0... ....  = Reserved 
    .... 10.. .... ....  = Destination Addressing Mode: Address field contains a 16-bit 
    short address    
    ..00 .... .... ....  = Reserved for Zigbee/Zigbee PRO/RF4CE, 
                                 Frame Ver. for Zigbee IP 
     00.. .... .... ....  = Source Addressing Mode: PAN identifier and address         
    field are not present 
    Sequence Number: xx 
    Destination PAN Identifier: 0xffff 
    Destination Address: 0xffff 
    MAC Payload 
    Command Frame Identifier = Beacon Request: (0x07) 
     Frame Check Sequence: Correct 
 
The DUT shall return 
MLME-BEACON-NOTIFY.indication( 
    BSN = xx, 
    PANDescriptor, 
    PendAddrSpec = 0x00 
    AddrList = NULL, 
    sduLength = 0x02, 
    sdu = 0x12 0x34 
) 

The DUT shall return a Success result, such that: 
In the case where macAutoRequest = True 
MLME-SCAN.confirm ( 
    Status=0x00=Success, 
    ScanType=0x01=Active Scan, 
    ChannelPage=0, 
    UnscannedChannels=0x00 00 00 00, 
    ResultListSize=01, 
    EnergyDetectList=NULL, 
    PANDescriptorList 
) 
 
PANDescriptorList: 
    CoordAddrMode=0x2  
    CoordPanId=0x1aaa 
    CoordAddr=0xbb00  
    LogicalChannel=0x14 
    ChannelPage=0 
    SuperframeSpec=0x4FFF 
    GTSPermit=FALSE 
    LinkQuality=0x00-0xff 
    Timestamp 
    SecurityFailure=SUCCESS 
    SecurityLevel=0 [ KeyIdMode = NULL 
    KeySource=NULL
    KeyIndex=0x00]
 
In the case where macAutoRequest = False 
MLME-BEACON-NOTIFY.indication( 
    BSN = xx, 
    PANDescriptor (see above), 
    PendAddrSpec = 0x00 
    AddrList = NULL, 
    sduLength = 0x00, 
    sdu = NULL 
) 
 
MLME-SCAN.confirm ( 
    Status=0x00=Success, 
    ScanType=0x01=Active Scan, 
    ChannelPage=0, 
    UnscannedChannels=0x00 00 00 00, 
    ResultListSize=0, 
    EnergyDetectList=NULL, 
    PANDescriptorList=0) 
