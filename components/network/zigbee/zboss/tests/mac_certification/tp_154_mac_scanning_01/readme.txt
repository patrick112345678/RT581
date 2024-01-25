
Test TP/154/MAC/SCANNING-01

Perform an energy detect scan on several channels to check correct EnergyDetectList is generated.


Pass:

The DUT shall return a Success result, such that:
MLME-SCAN.confirm (
                  Status=0x00=SUCCESS,
                  ScanType=0x00=ED Scan,
                  ChannelPage=0,
                  UnscannedChannels=0x00 00 00 00,
                  ResultListSize=16(for 2.4GHz O?QPSK PHY)
                  ResultListSize=11(for 868/915MHz BPSK PHY)
                  ResultListSize=14(for 920MHz FSK PHY),
                  EnergyDetectList=values representing the energy on the different channel,
                  PANDescriptorList=NULL
                  )

Fail:
DUT does not respond, or EnergyDetectList contains unexpected values
