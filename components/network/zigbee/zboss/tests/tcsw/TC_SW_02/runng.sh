#/* ZBOSS Zigbee software protocol stack
# *
# * Copyright (c) 2012-2020 DSR Corporation, Denver CO, USA.
# * http://www.dsr-zboss.com
# * http://www.dsr-corporation.com
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
    kill $zc_pid
    kill $zr_pid
    kill $zr_joiner_pid
    kill $PipePID
}

killch_ex() {
    killch
    echo Interrupted by user!
    exit 1
}

trap killch_ex TERM INT

echo "run ns"
../../../platform/devtools/nsng/nsng &
PipePID=$!
echo sim started, pid $PipePID
sleep 1

echo

echo "run th_zc"
./th_zc &
zc_pid=$!
wait_for_start th_zc
echo ZC STARTED OK
sleep 1

echo

echo "run th_zr"
./th_zr &
zr_pid=$!
wait_for_start th_zr
echo ZR STARTED OK
sleep 1

echo

echo "run th_zr_joiner"
./th_zr_joiner &
zr_joiner_pid=$!
wait_for_start th_zr_joiner
echo ZR STARTED OK
sleep 1

echo

sleep 40

echo shutdown...
killch

echo 'Now verify traffic dump, please!'
