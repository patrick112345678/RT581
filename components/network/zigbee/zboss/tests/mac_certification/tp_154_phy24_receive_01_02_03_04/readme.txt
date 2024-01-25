This test can be used for tests RECEIVER-01/02/03/04

Test TP/154/PHY24/RECEIVER-01

Tests that the DUT receiver is capable of receiving packets at the Specified Sensitivity Level in the
table below with a less than 1% PER for a packet with a 20 byte data payload (20 octet PSDU).

Specified Sensitivity Level: -85 dBm 

Test Procudure:
1. Adjust the output power of Signal Generator 
to generate an input level equal to the 
Specified Sensitivity Level at DUT receiver 
input. 
Measure PER while the Signal Generator 
transmits packets and DUT receiver receives 
packets on the channel being tested. 
Signal Generator shall generate at least 1000 
packets at each power level being tested. 

Pass: For each channel in the band of operation 
the PER of the DUT is less than 1%. 

Fail: For any of the channels in the band of 
operation the PER of the DUT is greater than 
or equal to 1%. 

2. Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 

Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 


Test Description:
dut.bin is Device Under Test (DUT), th.bin operates as a Test Harness (TH).
TH sends 1000 packets on each channel with 0.1 seconds delay per packet and 30 seconds delay per channel for recevier synchronization.
Upon receipt of the first packet per given channel DUT starts delayed routine to switch the channel.

Briefly, the test sequence is the following:
1. Start TH;
2. Start DUT;
3. TH and DUT switch to the 11th channel;
3. Delay 30 seconds;
4. Start packet TX on TH, packet RX on DUT
5. After all packets are TX on the given channel TX waits for 30 seconds and switches to the 12th channel
6. After given timeout (time for 1000 packets TX) DUT switches to the 12th channel
7. ... and so on, up to the 26th channel.


Test TP/154/PHY24/RECEIVER-02

Tests that the DUT receiver is capable of receiving packets at the Specified Sensitivity Level in the
table below with a less than 1% PER for a packet with a 20 byte data payload (20 octet PSDU).

Band - PHY                   Desired Channel Test Input Level
2.4 GHz O-QPSK                          -82 dBm

given a jamming signal at the Adjacent Channel Relative Jamming Level specified below on an
adjacent channel.

Adjacent Channel Relative Jamming Level = 0 dB

Test Procudure:
1. Adjust the output power of Signal Generator 1
to generate an input level equal to the
Desired Channel Test Input Level at DUT
receiver input.
Adjust the output power of Signal Generator 2
to generate an input level equal to the
Adjacent Channel Relative Jamming Level at
DUT receiver input.
Signal Generator 1 shall be transmitting in
the DUT receiving channel, and Signal
Generator 2 shall be transmitting in either the
high or low channel adjacent (+/- 5 MHz) to
the channel Signal Generator 1 is
transmitting in.
Measure PER while the Signal Generator
transmits packets and DUT receiver receives
packets on the channel being tested.
Signal Generator 1 and Signal Generator 2
shall generate at least 1000 packets at each
power being level tested.
For each channel that the DUT is tested on a
jamming signal on both high and low
adjacent channels shall be tested.

2. Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 

Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 

Pass: For each channel in the band of operation
both the high and low adjacent channel
jamming resistance PER of the DUT is less
than 1%.

Fail: For any of the channels in the band of
operation either the high or low adjacent
channel jamming resistance PER of the DUT is
greater than or equal to 1%.

Test TP/154/PHY24/RECEIVER-03

Tests that the DUT receiver is capable of receiving packets at the Desired Channel Test Input Level in
the table below with a less than 1% PER for a packet with a 20 byte data payload (20 octet PSDU),

Band - PHY                   Desired Channel Test Input Level
2.4 GHz O-QPSK                          -82 dBm

given a jamming signal at the Adjacent Channel Relative Jamming Level specified below on an
adjacent channel.

Adjacent Channel Relative Jamming Level = +30 dB

Test Procudure:
1.Adjust the output power of Signal Generator
1 to generate an input level equal to the
Desired Channel Test Input Level at DUT
receiver input.
Adjust the output power of Signal Generator
2 to generate an input level equal to the
Alternate Channel Relative Jamming Level
at DUT receiver input.
Signal Generator 1 shall be transmitting in
the DUT receiving channel, and Signal
Generator 2 shall be transmitting in either the
high or low channel alternate (+/- 10 MHz)
to the channel Signal Generator 1 is
transmitting in.
Measure PER while the Signal Generator
transmits packets and DUT receiver receives
packets on the channel being tested.
Signal Generator 1 and Signal Generator 2
shall generate at least 1000 packets at each
power being level tested.
For each channel that the DUT is tested on a
jamming signal on both high and low
alternate channels shall be tested.

2. Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 

Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 

Pass: For each channel in the band of operation
both the high and low alternate channel
jamming resistance PER of the DUT is less
than 1%.

Fail: For any of the channels in the band of
operation either the high or low alternate
channel jamming resistance PER of the DUT is
greater than or equal to 1%.


Test TP/154/PHY24/RECEIVER-04

Tests that the DUT receiver is capable of receiving packets at the Specified Maximum Input Power
Level in the table below with a less than 1% PER for a packet with a 20 byte data payload (20 octet
PSDU).

Band - PHY                   Specified Maximum Input Power Level
2.4 GHz O-QPSK                          -20 dBm

Test Procudure:
1.Adjust the output power of Signal Generator,
to generate an input level equal to the
Specified Maximum Input Power Level at
DUT receiver input.
Measure PER while the Signal Generator
transmits packets and DUT receiver receives
packets on the channel being tested.
Signal Generator shall generate at least 1000
packets at each power level being tested.

2. Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 

Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 

Pass: For each channel in the band of operation
the PER of the DUT is less than 1%.

Fail: For any of the channels in the band of
operation the PER of the DUT is greater than
or equal to 1%.
