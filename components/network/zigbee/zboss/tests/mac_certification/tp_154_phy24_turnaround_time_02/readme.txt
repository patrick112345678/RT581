Test TP/154/PHY24/TURNAROUND-TIME-02

Tests that the DUT has a RX-to-TX turnaround time equal to the specified amount in the table below. 

RX-to-TX Turnaround Time: 12 symbols 

Test Procudure:
1. Signal Generator transmits a packet and 
DUT receiver receives the packet. 

Signal Generator generates a Start Trigger at 
the end of generation on the Signal Analyzer. 

DUT Acks to signal received from Signal 
Generator. 

On the Signal Analyzer measure the delay 
between the Start Trigger and the start of 
reception of the Ack frame from the DUT. 

Pass: For each channel in the band of operation 
all of the RX-to-TX Turnaround measurements 
are between 12 and 12,75 symbols (that is 
between 192 and 204 microseconds for the 2.4 
GHz band of operation). 

Fail: For any channel in the band of operation 
any of the RX-to-TX Turnaround 
measurements are not between 12 and 12.75 
symbols (that is not between 192 and 204 
microseconds for the 2.4 GHz band of 
operation). 

2. Perform test for each channel in the band of 
operation, whose Center frequencies are 
given by the following: 

Center frequency: 
fc = 2405+5*(k-11), 
where k = 11 (0x0B) to 26 (0x1A) 


Test Description:
dut.bin is Device Under Test (DUT), th.bin operates as a Test Harness (TH).
DUT sends 10 packets on each channel with 1 seconds delay per packet and 30 seconds delay per channel for recevier synchronization.
Upon receipt of the first packet per given channel DUT starts delayed routine to switch the channel.

Briefly, the test sequence is the following:
1. Start TH;
2. Start DUT;
3. TH and DUT switch to the 11th channel;
3. Delay 30 seconds;
4. Start packet RX on TH, packet TX on DUT
5. After all packets are TX on the given channel DUT waits for 30 seconds and switches to the 12th channel
6. After given timeout (time for 10 packets TX) DUT switches to the 12th channel
7. ... and so on, up to the 26th channel.

 