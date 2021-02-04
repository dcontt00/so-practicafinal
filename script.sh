#!/bin/bash

pid1=$(pidof practicaFinal)
pid2=$(pidof script.sh)
echo Practica Final
echo $pid1
echo Script
echo $BASHPID

while :
do
	kill -10 $pid1
	sleep 1
	kill -12 $pid1
	sleep 1
	kill -13 $pid1
	sleep 1
done


