#/* ZBOSS Zigbee software protocol stack
# *
# * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
# * www.dsr-zboss.com
# * www.dsr-corporation.com
# * All rights reserved.
# *
# * This is unpublished proprietary source code of DSR Corporation
# * The copyright notice does not evidence any actual or intended
# * publication of such source code.
# *
# * ZBOSS is a registered trademark of Data Storage Research LLC d/b/a DSR
# * Corporation
# *
# * Commercial Usage
# * Licensees holding valid DSR Commercial licenses may use
# * this file in accordance with the DSR Commercial License
# * Agreement provided with the Software or, alternatively, in accordance
# * with the terms contained in a written agreement between you and
# * DSR.
#
# PURPOSE:
#

PIPE_NAME=&

wait_for_start() {
    nm=$1
    s=''
    while [ A"$s" = A ]
    do
        sleep 1
        if [ -f core ]
        then
            echo 'Somebody has crashed (ns?)'
            killch
            exit 1
        fi
        s=`grep Device zdo_${nm}*.log`
    done
    if echo $s | grep started
    then
        return
    else
        echo $s
        killch
        exit 1
    fi
}

killch() {
    kill $r1_pid
    kill $r2_pid
    kill $r3_pid
    kill $r4_pid
    kill $ed_pid
    kill $c_pid
    kill $PipePID
}

killch_ex() {
    killch
    echo Interrupted by user!
    exit 1
}

trap killch_ex TERM INT

#sh clean
#make

echo "run ns"
../../../platform/devtools/nsng/nsng &
PipePID=$!
echo sim started, pid $PipePID

sleep 1

echo "run th_zc"
./th_zc &
c_pid=$!
wait_for_start th_zc
echo ZC STARTED OK
sleep 1

echo

echo "run th_zr1"
./th_zr1 &
r1_pid=$!
wait_for_start th_zr1
echo ZR1 STARTED OK
sleep 1

echo

echo "run th_zr2"
./th_zr2 &
r2_pid=$!
wait_for_start th_zr2
echo ZR2 STARTED OK
sleep 1

echo

echo "run th_zr3"
./th_zr3 &
r3_pid=$!
wait_for_start th_zr3
echo ZR3 STARTED OK
sleep 1

echo

echo "run th_zr4"
./th_zr4 &
r4_pid=$!
wait_for_start th_zr4
echo ZR4 STARTED OK
sleep 1

echo

echo "run dut_zed"
./dut_zed &
ed_pid=$!
wait_for_start dut_zed
echo ZED STARTED OK
sleep 1

sleep 40

echo KILLING ZED
kill $ed_pid

echo "run dut_zed"
./dut_zed &
ed_pid=$!
wait_for_start dut_zed
echo ZED STARTED OK
sleep 1

sleep 40

echo shutdown...
killch

#sh conv.sh

echo 'Now verify traffic dump, please!'
