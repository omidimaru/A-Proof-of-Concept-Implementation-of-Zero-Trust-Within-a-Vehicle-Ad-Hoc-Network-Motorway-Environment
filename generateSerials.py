#!/usr/bin/python3

import random
import sys

carNum = int(sys.argv[1])
rsuNum = int(sys.argv[2])
maliciousNum = int(sys.argv[3])

carDict = {}
rsuDict = {}
malDict = {}

def generateSerialNumbers(filename):
	newSerialNo = ""
	for i in range(9):
			newSerialNo+=str(random.randint(0,9))

	if filename[0] == 'c':
		if carDict.get(newSerialNo) == None:
			carDict[newSerialNo] = newSerialNo
		else:
			generateSerialNumbers(filename)
	elif filename[0] == 'r':
		if (rsuDict.get(newSerialNo) == None) & (carDict.get(newSerialNo) == None):
			rsuDict[newSerialNo] = newSerialNo
		else:
			generateSerialNumbers(filename)
	else:
		if (malDict.get(newSerialNo) == None) & (carDict.get(newSerialNo) == None) & (rsuDict.get(newSerialNo) == None):
			malDict[newSerialNo] = newSerialNo
		else:
			generateSerialNumbers(filename)

for i in range(carNum):
	generateSerialNumbers("carSerials.txt")

for i in range(rsuNum):
	generateSerialNumbers("rsuSerials.txt")

if maliciousNum > 0:
	for i in range(maliciousNum):
		generateSerialNumbers("maliciousSerials.txt")

print("Vehicles:")
print(carDict)
print("RSUs:")
print(rsuDict)
if maliciousNum > 0:
	print("Malicious Nodes:")
	print(malDict)

with open("carSerials.txt", "a") as f1, open("rsuSerials.txt", "a") as f2, open("maliciousSerials.txt", "a") as f3:
	f1.truncate(0)
	f2.truncate(0)
	f3.truncate(0)
	carDictEnd = list(carDict)[-1]
	rsuDictEnd = list(rsuDict)[-1]
	if maliciousNum > 0:
		malDictEnd = list(malDict)[-1]
	for serialNo in carDict:
		if serialNo == carDictEnd:
			f1.write(serialNo)
			break
		f1.write(serialNo+"\n")

	for serialNo in rsuDict:
		if serialNo == rsuDictEnd:
			f2.write(serialNo)
			break
		f2.write(serialNo+"\n")

	if maliciousNum > 0:
		for serialNo in malDict:
			if serialNo == malDictEnd:
				f3.write(serialNo)
				break
			f3.write(serialNo+"\n")

	f1.close()
	f2.close()
	f3.close()
