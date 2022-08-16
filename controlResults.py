#!/usr/bin/python3

import re
import sys
import math

simMulti = float(sys.argv[2])
malNum = sys.argv[1]
evenSimTime, oddSimTime = 39, 63

# i is the number of nodes in each simulation results file
for i in range(10, 101, 10):
	numNode = i
	simTime = (i * simMulti) + 63 + 3
	oddNodes = evenNodes = i/2
	sentpackets = 0
	if simMulti == 1.0:
		simMulti = int(1)
	resultsFile = str(numNode) + 'v' + malNum + 'm' + str(simMulti) + 's'

	print("********************* processing " + str(i) + " file *********************")
	with open("/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/" + resultsFile) as f:
		datafile = f.readlines()

		delays = []
		averageDelay = 0

		thrpEven = thrpOdd = 0
		evenBytesThrp = oddBytesThrp = 0
		totalRx = totalPkt = 0
		evenBytes = oddBytes = 0

		totalRegRTT = totalRegistrations = 0

		totalGP = trustedGP = malGP = 0
		evenTotalGP = evenTrustedGP = evenMalGP = 0
		oddNodeGP = oddTrustedGP = oddMalGP = 0

		dosdelay = dospkts = normdelay = normpkts = 0

		posEvent = negEvent = forgotten = numTrustOpinion = avgTrustedVehicles = 0
		avgFaith = avgSuspicion = avgUncertainty = avgPosEvents = avgNegEvents = 0.0
		badVehiclesPerNode = totalBadCount = 0
		badVehicles = []
		badVehFirstContact = badVehDecision = 0

		for line in datafile:
			#delay in ms
			if re.search(r'\bdelay for node\b\s[0-9]*:\s[0-9]*\.?[0-9]*', line):
				pktDelay = line.split()[-1]
				delays.append(pktDelay)

			#throughput in bytes
			elif re.search('Bps\sfor\snode\s[0-9]*\s:\s[0-9]*\.?[0-9]*?', line):
				stats = line.split()
				if int(stats[-3]) % 2 == 0:
					evenBytesThrp += float(stats[-1])
				else:
					oddBytesThrp += float(stats[-1])

			elif re.search('polywag\snode\s[0-9]*\s[0-9]*\s[0-9]*', line):
				rip = line.split()
				totalRx += float(rip[-2])
				totalPkt += float(rip[-1])

			#registration RTT
			elif re.search('registration\srtt\s[0-9]*.?[0-9]*?', line):
				totalRegRTT += float(line.split()[-1])
				totalRegistrations += 1

			#network/node goodput in bytes
			elif re.search(r'\bTotal Goodput, Trusted Goodput, Malicious Goodput - Node\b\s[0-9]*\s:\s[0-9]*\s[0-9]*\s[0-9]*', line):
				totalGP += int(line.split()[-3])
				trustedGP += int(line.split()[-2])
				malGP += int(line.split()[-1])
				if int(line.split()[-5]) % 2 == 0:
					evenTotalGP += int(line.split()[-3])
					evenTrustedGP += int(line.split()[-2])
					evenMalGP += int(line.split()[-1])
				else:
					oddNodeGP += int(line.split()[-3])
					oddTrustedGP += int(line.split()[-2])
					oddMalGP += int(line.split()[-1])

			elif re.search('dos\sdelay:\s[0-9]*.?[0-9]*?\s[0-9]*', line):
				dosdelay += float(line.split()[-2])
				dospkts += int(line.split()[-1])

			elif re.search('norm\sdelay:\s[0-9]*.?[0-9]*?\s[0-9]*', line):
				normdelay += float(line.split()[-2])
				normpkts += int(line.split()[-1])

			#trust level avg
			elif re.search(r'gvehicle\s[0-9]*\spositive([0-9])*\snegative([0-9])*', line):
				goodSplit = line.split("e")
				x = int(line.split("e")[-1])
				y = int(goodSplit[-3].split()[-2])
				negEvent += x
				posEvent += y
				if(x == 0 and y == 0):
					forgotten += 1
				numTrustOpinion += 1

			#number of nodes thought malicious
			elif re.search(r'bvehicle\s[0-9]*\s[0-9]*\s[0-9]*', line):
				if line.split()[-3] not in badVehicles:
					badVehicles.append(line.split()[-3])
				totalBadCount += 1
				#bad vehicle decision time
				badVehFirstContact += int(line.split()[-1])
				badVehDecision += int(line.split()[-2])


		f.close()

		for delay in delays:
			averageDelay += float(delay)

		#Delay in ms
		averageDelay = averageDelay/len(delays)
		print(len(delays))
		print("Average packet delay in ms: " + str(averageDelay))
		delay_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgDelayMs-' + malNum + 'm' + str(simMulti) + 's', 'a')
		delay_f.write(str(numNode) + ' ' + str(averageDelay) + '\n')
		delay_f.close()

		#Throughput in Mbps per node
		thrpEven = (((evenBytesThrp * 8)/1000000) / evenSimTime) / evenNodes
		thrpOdd  = (((oddBytesThrp * 8)/1000000) / oddSimTime) / oddNodes
		#print(thrpEven)
		#print(thrpOdd)
		avThrp = (thrpEven + thrpOdd) / 2
		print("Throughput Average Per Node (39s/63s): " + str(avThrp))
		nodeThrp_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgNodeThroughputMbps-' + malNum + 'm' + str(simMulti) + 's', 'a')
		nodeThrp_f.write(str(numNode) + ' ' + str(avThrp) + '\n')
		nodeThrp_f.close()

		#Throughput in Mbps for whole network
		totalRxBits = (evenBytesThrp + oddBytesThrp) * 8
		#print("totalRxBytes: " + str(totalRx))
		#print("totalRx bits: " + str(totalRxBits))
		#print("totalPkt: " + str(totalPkt))
		networkThrp = (totalRxBits / 1000000) / simTime
		print("Total Network Throughput Mbps (" + str(math.floor(simTime)) + "): " + str(networkThrp))
		networkThrp_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgNetworkThroughputMbps-' + malNum + 'm' + str(simMulti) + 's', 'a')
		networkThrp_f.write(str(numNode) + ' ' + str(networkThrp) + '\n')
		networkThrp_f.close()

		#Average RSU registration time
		avgRegRTT = totalRegRTT / totalRegistrations
		print("Average Registration time: " + str(avgRegRTT))
		resuRegTime_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgRsuRegTime-' + malNum + 'm' + str(simMulti) + 's', 'a')
		resuRegTime_f.write(str(numNode) + ' ' + str(avgRegRTT) + '\n')
		resuRegTime_f.close()

		#Network goodput
		networkGoodput = ((totalGP * 8) / 1000000 ) / simTime
		networkTrustedGoodput = ((trustedGP * 8) / 1000000 ) / simTime
		networkMalGoodput = ((malGP * 8) / 1000000 ) / simTime
		print("Average Network Goodput: " + str(networkGoodput))
		print("Average Network Trusted Goodput: " + str(networkTrustedGoodput))
		print("Average Network Malicious Goodput: " + str(networkMalGoodput))
		networkGP_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgNetworkGoodput-' + malNum + 'm' + str(simMulti) + 's', 'a')
		networkGP_f.write(str(numNode) + ' ' + str(networkGoodput) + ' ' + str(networkTrustedGoodput) + ' ' + str(networkMalGoodput) + '\n')
		networkGP_f.close()

		#Even node goodput - only in network for 39 seconds
		totalGPEven = (((evenTotalGP * 8) / 1000000 ) / evenSimTime ) / evenNodes
		trustGPEven = (((evenTrustedGP * 8) / 1000000 ) / evenSimTime ) / evenNodes
		malGPEven = (((evenMalGP * 8) / 1000000 ) / evenSimTime ) / evenNodes
		print("Average Even Node (39s): Total Goodput, Trusted Goodput, Malicious Goodput - " + str(totalGPEven) + " " + str(trustGPEven) + " " + str(malGPEven))
		#Odd node goodput - only in network for 63 seconds
		totalGPOdd = (((oddNodeGP * 8) / 1000000 ) / oddSimTime ) / oddNodes
		trustGPOdd = (((oddTrustedGP * 8) / 1000000 ) / oddSimTime ) / oddNodes
		malGPOdd = (((oddMalGP * 8) / 1000000 ) / oddSimTime ) / oddNodes
		print("Average Odd Node (63s): Total Goodput, Trusted Goodput, Malicious Goodput - " + str(totalGPOdd) + " " + str(trustGPOdd) + " " + str(malGPOdd))
		nodeGP_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgNodeGoodput-' + malNum + 'm' + str(simMulti) + 's', 'a')
		nodeGP_f.write(str(numNode) + ' ' + str(totalGPEven) + ' ' + str(trustGPEven) + ' ' + str(malGPEven) + ' ' + str(totalGPOdd) + ' ' + str(trustGPOdd) + ' ' + str(malGPOdd) + '\n')
		nodeGP_f.close()

		#dosdelay and normDelay
		avgDosDelay = dosdelay / numNode
		avgNormDelay = normdelay / numNode
		print("DoS delay avg: " + str(avgDosDelay))
		print("Norm delay avg: " + str(avgNormDelay))
		splitDelay_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgSplitDelay-' + malNum + 'm' + str(simMulti) + 's', 'a')
		splitDelay_f.write(str(numNode) + ' ' + str(avgNormDelay) + ' ' + str(avgDosDelay) + '\n')
		splitDelay_f.close()

		#trust level average
		avgTrustedVehicles = float((numTrustOpinion - forgotten) / numNode)
		avgPosEvents = float(posEvent / avgTrustedVehicles)
		avgNegEvents = float(negEvent / avgTrustedVehicles)
		avgFaith = float((avgPosEvents / (avgPosEvents + avgNegEvents + 2)))
		avgSuspicion = float((avgNegEvents / (avgPosEvents + avgNegEvents + 2)))
		avgUncertainty = float((2 / (avgPosEvents + avgNegEvents + 2)))
		print("Avg Faith: " + str(round(avgFaith, 2)))
		print("Avg Suspicion: " + str(round(avgSuspicion, 2)))
		print("Avg Uncertainty: " + str(round(avgUncertainty, 2)))
		avgTrustLevel_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgTrustLevel-' + malNum + 'm' + str(simMulti) + 's', 'a')
		avgTrustLevel_f.write(str(numNode) + ' ' + str(avgFaith) + ' ' + str(avgSuspicion) + ' ' + str(avgUncertainty) + '\n')
		avgTrustLevel_f.close()

		#bad vehicles per node and unique bad vehicles
		badVehiclesPerNode = totalBadCount / numNode
		numBadNodes = len(badVehicles)
		print("Bad Vehicles per Node: " + str(round(badVehiclesPerNode, 2)))
		print("Num Unique Bad Vehicles: " + str(round(numBadNodes, 2)))
		avgBadVehicle_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgBadVehicles-' + malNum + 'm' + str(simMulti) + 's', 'a')
		avgBadVehicle_f.write(str(numNode) + ' ' + str(badVehiclesPerNode) + ' ' + str(numBadNodes) + '\n')
		avgBadVehicle_f.close()

		#bad vehicle decision (ms)
		avgBadGuyDecision = 0 if (badVehDecision - badVehFirstContact) == 0 else (badVehDecision - badVehFirstContact) / totalBadCount
		print("Avg BadVehicle decision made after (ms): " + str(round(avgBadGuyDecision, 2)))
		avgBadGuyDecision_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgBadGuyDecision-' + malNum + 'm' + str(simMulti) + 's', 'a')
		avgBadGuyDecision_f.write(str(numNode) + ' ' + str(avgBadGuyDecision) + '\n')
		avgBadGuyDecision_f.close()



	#Packet Delivery Ratio
	packetDict = {}
	numSent = 0
	totalPktRcv = 0
	avgVehiclesReceived = 0

	with open("/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/control." + resultsFile + "-waveReceive.txt") as f:
		datafile = f.readlines()

		for line in datafile:
			uid = int(line.split()[-1])
			if uid not in packetDict:
				packetDict[uid] = 1
			else:
				packetDict[uid] += 1

		for uid in packetDict:
			totalPktRcv += packetDict[uid]
	f.close()

	with open("/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/control." + resultsFile + "-waveSent.txt") as f:
		datafile = f.readlines()
		for line in datafile:
			numSent += 1
	f.close()

	avgVehiclesReceived = round(((totalPktRcv - dospkts)/ numSent), 2)
	print("Average number of vehicles to receive a packet: " + str(avgVehiclesReceived))
	packetDelivRatio = ((totalPktRcv - dospkts) / numSent) / avgVehiclesReceived
	if packetDelivRatio > 1:
		packetDelivRatio = 1
	print("Average Packet Delivery Ratio: " + str(packetDelivRatio))
	print("Total packets rx: " + str(totalPktRcv) + " :: DoS Packets rx: " + str(dospkts))
	pdr_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgNumNodeRecv-' + malNum + 'm' + str(simMulti) + 's', 'a')
	pdr_f.write(str(numNode) + ' ' + str(avgVehiclesReceived) + '\n')
	pdr_f.close()
	print(numSent)
	pdr_f = open('/home/ben/ns-allinone-3.36.1/ns-3.36.1/results/control/plotfiles/' + malNum + 'm' + str(simMulti) + 's/AvgPacketDeliveryRatio-' + malNum + 'm' + str(simMulti) + 's', 'a')
	pdr_f.write(str(numNode) + ' ' + str(packetDelivRatio) + '\n')
	pdr_f.close()
