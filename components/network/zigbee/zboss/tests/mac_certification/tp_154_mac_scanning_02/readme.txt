
Test TP/154/MAC/SCANNING-02

Perform an active scan on several channels to check correct negative response when coordinator(s) not present

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
        .... 10.. .... ....  = Destination Addressing Mode: Address field contains a 16-
        bit short address    
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

The DUT shall return a Success result, such that: 
MLME-SCAN.confirm ( 
    Status=0xea="NO_BEACON",
    ScanType=0x01=Active Scan, 
    ChannelPage=0,
    UnscannedChannels=0x00 00 00 00, 
    ResultListSize=0, 
    EnergyDetectList=NULL, 
    PANDescriptorList=NULL ) 
 
Fail:  
DUT does not respond, or DUT responds with SUCCESS 
