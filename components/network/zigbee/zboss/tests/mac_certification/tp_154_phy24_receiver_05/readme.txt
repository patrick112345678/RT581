TP/154/PHY24/RECEIVER-05

Tests that the DUT is capable of reporting Energy Detection (ED) values with the proper monotonicity 
and linearity requirements. 

The range of detection shall span at least 40 dB above the highest power level at which the DUT 
reports an ED value of zero and with a 6 dB accuracy. The ED measurement time, to average over, 
shall be equal to 8 symbol perio

Note:
when trace logs from this test are available, run this to get table-like summary of scan measurements:

cut -f 2- zdo_dut.log | grep PHY | sed 's/^PHY: //g' | paste -d" "  - - - - - - - - - - -
Step-by-step:
1) cut message numbers and timestamp columns
2) grep for measurement trace
3) Remove "PHY: " prefix from beginning of every string
4) Concat lines from stdin by 11-line groups (stdin is given as parameter to paste 11 times)

Example resuls 
channel 11 expected power offset 0 values= 0 0 0 0 0 0 0 0 0 0
channel 11 expected power offset 2 values= 0 0 0 0 0 0 0 0 0 0
