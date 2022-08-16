#!/usr/bin/python3

import re

with open("/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/waveReceive.txt") as f:
    datafile = f.readlines()

    packetDict = {}
    numUniqPkt = 0
    totalPktRcv = 0
    avgVehiclesReceived = 0

    for line in datafile:
        uid = int(line.split()[-1])
        if uid not in packetDict:
            packetDict[uid] = 1
        else:
            packetDict[uid] += 1

    for uid in packetDict:
        totalPktRcv += packetDict[uid]
        numUniqPkt += 1

avgVehiclesReceived = round((totalPktRcv / numUniqPkt), 2)
print("Average number of vehicles to receive a packet: " + str(avgVehiclesReceived))
