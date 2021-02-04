#!/bin/bash

pid=$(pidof practicaFinal)
echo $pid
while :
do
	kill -10 $pid
	sleep 1
	kill -12 $pid
	sleep 1
	kill -13 $pid
	sleep 1
done


