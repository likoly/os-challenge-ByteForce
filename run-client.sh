#!/bin/sh

PATHTOCOMMON=/home/vagrant/os-challenge-common
SERVER=192.168.101.10
PORT=5002
SEED=1234
TOTAL=20
START=0
DIFFICULTY=8
REP_PROB_PERCENT=0
DELAY_US=100000
PRIO_LAMBDA=0

$PATHTOCOMMON/$(./get-bin-path.sh)/client $SERVER $PORT $SEED $TOTAL $START $DIFFICULTY $REP_PROB_PERCENT $DELAY_US $PRIO_LAMBDA
