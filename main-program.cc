#include "ns3/vector.h"
#include "ns3/string.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/double.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/command-line.h"
#include "ns3/position-allocator.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include <iostream>

#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/core-module.h"

#include <string>
#include <map>
#include <regex>
#include "VehicleApp.h"
#include "RsuApp.h"
#include "WaveApp.h"
#include "DosApp.h"
#include "SignatureHeader.h"
#include "ns3/config-store.h"
#include "ns3/ipv4.h"
#include "TargetHeader.h"
#include "ns3/ethernet-header.h"
#include "ns3/wave-module.h"
#include "ns3/wave-helper.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/seq-ts-header.h"

#include "ns3/netanim-module.h"

#include "ns3/command-line.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"

//flowmonitor stuff for packet tracking - only really useful for unicast packet performance stats
//#include "ns3/flow-monitor.h"
//#include "ns3/flow-monitor-helper.h"

using namespace ns3;

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

uint32_t countDrop = 0;
uint32_t countReceived = 0;
uint32_t waveDrop = 0;
uint32_t waveReceived = 0;
uint32_t waveTransmit = 0;
uint32_t waveRecvEnd = 0;
uint32_t waveTransEnd = 0;

NS_LOG_COMPONENT_DEFINE("attempt");
//NS_OBJECT_ENSURE_REGISTERED(attempt);

void RxDrop(std::string context, Ptr<const Packet> packet, WifiPhyRxfailureReason reason){
	//PREAMBLE_DETECT_FAILURE just means a vehicle is out of range of the broadcast - not really something that should matter
	if(reason != ns3::PREAMBLE_DETECT_FAILURE){
		WifiMacHeader wmh;
		packet->PeekHeader(wmh);
		if(wmh.GetAddr1() != "ff:ff:ff:ff:ff:ff") {
			NS_LOG_DEBUG(BOLD_CODE << YELLOW_CODE << "DROPPED A PACKET - " << reason << " - " << packet->ToString() << END_CODE);
			//writes the dropped packets to the testAppend.txt file
			//used to make sure all packets are out-of-range ARP requests
			// std::ofstream outfile;
			// outfile.open("results/rsuDrops.txt", std::ios_base::app);
			// outfile << " wifimacheader address 1: " <<  wmh.GetAddr1() << " ";
			// outfile << "Reason: " << reason << " - Packet: ";
			// outfile << packet->ToString() << "\n\n";

			countDrop++;
			// outfile.close();
		}
	}
}

void RxBegin(std::string context, Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW){
	std::ofstream outfile;
	//outfile.open("results/receivedPackets.txt", std::ios_base::app);
	//outfile << "Packetpal: ";
	//outfile << packet->ToString() << "\n\n";
	//std::cout << BOLD_CODE << YELLOW_CODE << "RECEIVED A PACKET";
	//std::cout << packet->ToString();
	//std::cout << END_CODE << std::endl;
	countReceived++;
	// outfile.close();
}

void WaveRxDrop (std::string context, Ptr<const Packet> packet, ns3::WifiPhyRxfailureReason reason){
	//PREAMBLE_DETECT_FAILURE just means a vehicle is out of range of the broadcast - again, shouldn't count as dropped in my opinion
	if(reason != PREAMBLE_DETECT_FAILURE){
		NS_LOG_DEBUG(BOLD_CODE << YELLOW_CODE << "DROPPED A WAVE PACKET - " << reason << " - " << packet->ToString() << END_CODE);

		// Ptr<Packet> packetCopy = packet->Copy();
		// SignatureHeader sig;
		// SeqTsHeader seq;
		// packetCopy->RemoveHeader(sig);
		// packetCopy->PeekHeader(seq);


		// TargetTag blah;
		// packet->PeekPacketTag(blah);
		//writes the dropped packets to the testAppend.txt file
		//used to make sure all packets are out-of-range ARP requests
		// std::ofstream outfile;
		// outfile.open("results/waveDrops.txt", std::ios_base::app);
		// outfile << "Context: " << context << " - Reason: " << reason << " - Packet: ";
		// outfile << packet->ToString() << " ";
		// blah.Print(outfile);
		// outfile << " timestamp: " << seq.GetTs().GetTimeStep();
		// outfile << "\n\n";

		waveDrop++;
		// outfile.close();

	}
}

void WaveRxBegin(std::string context, Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW){
	waveReceived++;

	// std::ofstream outfile;
	// outfile.open("results/waveReceive.txt", std::ios_base::app);
	// outfile << "Packet UID: " <<  packet->GetUid() << "\n";
	// outfile.close();
}

void WaveRxEnd(std::string context, Ptr<const Packet> packet){
	waveRecvEnd++;
}

void WaveTxBegin (std::string context, Ptr <const Packet> packet, double txPowerW){
	//std::cout << "wave packet transmit size: " << packet->GetSize() << " with power: " << txPowerW << std::endl;
    waveTransmit++;
}

void WaveTxEnd (std::string context, Ptr <const Packet> packet){
	//std::cout << "wave packet transmit size: " << packet->GetSize() << " with power: " << txPowerW << std::endl;
    waveTransEnd++;
}

int main (int argc, char *argv[]) {

	std::string phyMode ("OfdmRate6MbpsBW10MHz");

	//some simulation settings
	std::string filename = "";
	uint32_t vehicleNum = 120;
	uint32_t rsuNum = 3;
	uint32_t maliciousNum = 1;
	double startMultiplicative = 1.0; //used to control how close the vehicles are together
	//double simTime = (vehicleNum * startMultiplicative) + 63.0 + 3.0;
	//		1 = 66m /  42m
	//	  0.5 = 33m / 21m
	//   0.36 = 23.76m / 15.12m

	//this command is NEEDED so that any packet->ToString() calls actually show information.
	PacketMetadata::Enable ();
	//Packet::EnablePrinting(); can also be used

	CommandLine cmd;
	cmd.AddValue("vehi","The amount of vehicles", vehicleNum);
	cmd.AddValue("mal","Change how many DoS nodes there are - up to 2", maliciousNum);
	cmd.AddValue("multi","Change how dense the vehicles are", startMultiplicative);
	cmd.AddValue("filename","Results filename prefix", filename);
	cmd.Parse (argc, argv);

	uint32_t nodeNum = vehicleNum + rsuNum + maliciousNum;
	double simTime = (vehicleNum * startMultiplicative) + 63.0 + 3.0;
	//		1 = 66m /  42m
	//	  0.5 = 33m / 21m
	//   0.36 = 23.76m / 15.12m

 	// RSU broadcast message interval
 	Time bInterval = MilliSeconds (50);

	//create the nodes
	NodeContainer nodes;
	nodes.Create(nodeNum);

	//std::cout << nodes.Get(0).TypeId() << std::endl;

	// create mobility
	// everything needs a position, even if it's constant
	MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	//mobility.SetMobilityModel ("ns3::ConstantVelocityHelper");
    mobility.Install(nodes);

	//set the 3 RSU node's position manually - all 3 slightly above ground as if they were raised antennas
	Ptr<ConstantVelocityMobilityModel> rsu1 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(0)->GetObject<MobilityModel>());
	rsu1->SetPosition ( Vector (0, 0, 10));
	rsu1->SetVelocity (Vector (0, 0, 0));
	Ptr<ConstantVelocityMobilityModel> rsu2 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(1)->GetObject<MobilityModel>());
	rsu2->SetPosition (Vector (390, 0, 10));
	rsu2->SetVelocity (Vector (0, 0, 0));
	Ptr<ConstantVelocityMobilityModel> rsu3 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(2)->GetObject<MobilityModel>());
	rsu3->SetPosition (Vector (780, 0, 10));
	rsu3->SetVelocity (Vector (0, 0, 0));

	//the velocity in NS3 is measured in m/s
	//this leads 33m/s = 120km/h and 22.3m/s = 80km/h
    for (uint32_t i = rsuNum; i < nodes.GetN(); i++){
		//double accl = (i % 2 == 1) ? 20 : 33;
	    //set initial positions - velocity gets set at VehicleApp start time
		Ptr<ConstantVelocityMobilityModel> cvmm = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(i)->GetObject<MobilityModel>());
		cvmm->SetPosition ( Vector (-600, 20 + (i % 2) * 5, 1));
    }


	/*
	*  Need to set up IP addresses for each node with YansWifi to allow easier communications between single points.
	*  WAVE networks are generally based around broadcasting safety messages, but for ZTA we need to control the
	*  access to the network, meaning the RSU needs to be contactable directly.
	*  Can be done with a WAVE network setup too, but to communicate directly I'd need to set up some complicated
	*  UDP ACK replies and mess with adding and dealing with custom packet headers. Without these acks, the unicast_packet
	*  MAC packets are replayed 6/7 times as the sender doesn't get a response from the receiver to let them know the packets
	*  came through.
	*/
	YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    Ptr<YansWifiChannel> channel = wifiChannel.Create ();
    wifiPhy.SetChannel (channel);
	wifiPhy.Set ("TxPowerStart", DoubleValue (33)); //	33 is the max dBm for non-government services    -     gives about a 200m radius around RSU
	wifiPhy.Set ("TxPowerEnd", DoubleValue (33)); // 	44.8 dBm max for government use                  -     gives around 460m radius
	wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
    // ns-3 supports generate a pcap trace
    wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
    QosWaveMacHelper wifi80211pMac = QosWaveMacHelper::Default ();
    Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

	wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                        "DataMode",StringValue (phyMode),
                                        "ControlMode",StringValue (phyMode),
										"NonUnicastMode", StringValue (phyMode));
	NetDeviceContainer wifiDevices = wifi80211p.Install(wifiPhy, wifi80211pMac, nodes);
	//wifiPhy.EnablePcap("attempt", wifiDevices);

	// Create the internet stack for each node's device
	InternetStackHelper internet;
    internet.Install (nodes);

    Ipv4AddressHelper ipv4;
    //NS_LOG_INFO ("Assign IP Addresses.");
    ipv4.SetBase ("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer ipv4Container = ipv4.Assign(wifiDevices);












	YansWavePhyHelper wavePhy = YansWavePhyHelper::Default();
	YansWifiChannelHelper waveChannel = YansWifiChannelHelper::Default();
	wavePhy.SetChannel(waveChannel.Create());
	wavePhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
	//set these two TxPower start/end values to change the transmission power from the vehicles
	//seems to only use the start power when deciding transmission power level
	wavePhy.Set ("TxPowerStart", DoubleValue (33) );
	wavePhy.Set ("TxPowerEnd", DoubleValue (33) );
	wavePhy.Set ("TxPowerLevels", UintegerValue (1));

	QosWaveMacHelper waveMac = QosWaveMacHelper::Default();
	WaveHelper waveHelper = WaveHelper::Default();

	waveHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
						  "DataMode", StringValue (phyMode),
						  "ControlMode",StringValue (phyMode),
						  "NonUnicastMode", StringValue (phyMode));

	NetDeviceContainer waveDevices = waveHelper.Install (wavePhy, waveMac, nodes);


	//wavePhy.EnablePcap("project-wave", waveDevices);












	//this sets up sockets on all of the vehicle nodes, node zero/one/two (0/1/2) being the rsus
	//the rsu socket is set manually, following this
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

	const SchInfo m_cchInfo = SchInfo (CCH, false, EXTENDED_ALTERNATING);
	const SchInfo m_sch1Info = SchInfo (SCH1, false, EXTENDED_ALTERNATING);


	//create the vehicle nodes, attach a UDPSocket, VehicleApp, WaveApp
	std::ifstream carFile ("scratch/project/carSerials.txt");
	for(uint16_t i = rsuNum; i < (nodes.GetN() - maliciousNum); i++){
		std::string data;
		carFile >> data;
		uint32_t serial = std::stoul(data);

	    Ptr<UdpSocket> vehicleSocket = DynamicCast<UdpSocket>(Socket::CreateSocket (nodes.Get (i), tid));

		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
		vehicleSocket->Bind(local);
		vehicleSocket->BindToNetDevice(nodes.Get(i)->GetDevice(0));
		// std::cout << nodes.Get(i)->GetDevice(0)->GetAddress() << " is the net address within app."<< std::endl;
		// std::cout << "\t" << nodes.Get(i)->GetDevice(2)->GetInstanceTypeId() << std::endl;
																		//    net devices				wave net devices
		Ptr<VehicleApp> app_i = CreateObject<VehicleApp>(vehicleSocket, nodes.Get(i)->GetDevice(0), DynamicCast<WaveNetDevice>(nodes.Get(i)->GetDevice(2)), ipv4Container, serial);
		Ptr<WaveApp> app_j = CreateObject<WaveApp>(nodes.Get(i)->GetDevice(2), serial, nodes, maliciousNum, filename);
		//removed making a wave net device pointer

		//change to different values to shorten or widen the distance between the vehicles
		//		1 = 66m / 42m
		//	  0.5 = 33m / 21m
		//   0.36 = 23.76m / 15.12m
		double startTime = double(i) * startMultiplicative;
		//takes roughly 46 seconds for a node from start time to traverse the network at 27.7m/s accl in VehicleApp - roughly 100km/h
		//takes 63 seconds at 21m/s and 39 seconds at 33 m/s
		//takes roughly 4 seconds to get into range of the first RSU which then enables packet broadcasting
		//stop application at total sim time - 3seconds to allow for the printing and manipulation of data to happen before being destroyed
		//double evenNodeEnd = ((simTime - 3) <= (startTime + 63.0)) ? simTime - 3 : (startTime + 63.0);
		double evenNodeEnd = ((simTime - 3) <= (startTime + 39.0)) ? simTime - 3 : (startTime + 39.0);
		double oddNodeEnd = ((simTime - 3) <= (startTime + 63.0)) ? simTime - 3 : (startTime + 63.0);

		app_i->SetStartTime(Seconds(startTime));
		app_j->SetStartTime(Seconds(startTime));

		if(i % 2 == 0){
			app_i->SetStopTime(Seconds(evenNodeEnd));
			app_j->SetStopTime(Seconds(evenNodeEnd));
		}else{
			app_i->SetStopTime(Seconds(oddNodeEnd));
			app_j->SetStopTime(Seconds(oddNodeEnd));
		}

		nodes.Get(i)->AddApplication(app_i);
		nodes.Get(i)->AddApplication(app_j);
	}
	carFile.close();


	//sets up the RSU nodes (0/1/2) with their Socket and RsuApp
	std::ifstream rsuFile ("scratch/project/rsuSerials.txt");
	for(uint16_t i = 0; i < rsuNum; i++){
		std::string data;
		rsuFile >> data;
		uint32_t rsuSerial = std::stoul(data);

		Ptr<Socket> rsu = Socket::CreateSocket (nodes.Get(i), tid);
		InetSocketAddress localAddress = InetSocketAddress (Ipv4Address::GetAny (), 80);
		InetSocketAddress broadcastZone = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);

		rsu->Bind(localAddress);
		rsu->BindToNetDevice(nodes.Get(i)->GetDevice(0));

		rsu->SetAllowBroadcast (true);
		Ptr<RsuApp> rsuApp = CreateObject<RsuApp>(rsu, nodes.Get(i)->GetDevice(0), ipv4Container, rsuSerial);

		rsuApp->SetStartTime(Seconds(i + 0.2));
		rsuApp->SetStopTime(Seconds(simTime));
		nodes.Get(i)->AddApplication(rsuApp);

		Ptr<Packet> packet = Create <Packet>(200);
		//this is the broadcast header to let the vehicles know which RSU zone it is and who to contact
		TargetHeader targetHeader = TargetHeader(uint32_t (21), i, rsuApp->getSerialNo());
		packet->AddHeader(targetHeader);

		//the RSUs need to broadcast in a staggered fashion, otherwise a combative deadzone appears between the two RSUs and effectively
		//reduces their broadcast range
		Simulator::Schedule(bInterval+Seconds(i), &RsuApp::sendPacket, rsuApp, packet, broadcastZone, localAddress, bInterval);
	}
	rsuFile.close();


	if(maliciousNum > 0){
		//set up the malicious nodes to be opposite side of road to first and last RSU units
		if(maliciousNum == 1){
			Ptr<ConstantVelocityMobilityModel> malicious1 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(nodes.GetN() - 1)->GetObject<MobilityModel>());
			malicious1->SetPosition (Vector (0, 40, 10));
			malicious1->SetVelocity (Vector (0, 0, 0));

			Ptr<WaveNetDevice> device = DynamicCast<WaveNetDevice>(nodes.Get(nodes.GetN() - 1)->GetDevice(2));
			Ptr<DosApp> dosApp = CreateObject<DosApp>(device, 666);

			dosApp->SetStartTime(Seconds(0));
			dosApp->SetStopTime(Seconds(simTime - 5));
			nodes.Get(nodes.GetN() - 1)->AddApplication(dosApp);

			Simulator::Schedule(Seconds(1), &DosApp::spam, dosApp, 20);
		}
		if(maliciousNum == 2){
			//puts a DoS node morroring the first and last RSUs in the network
			Ptr<ConstantVelocityMobilityModel> malicious1 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(nodes.GetN() - 2)->GetObject<MobilityModel>());
			malicious1->SetPosition (Vector (0, 40, 10));
			malicious1->SetVelocity (Vector (0, 0, 0));
			Ptr<ConstantVelocityMobilityModel> malicious2 = DynamicCast<ConstantVelocityMobilityModel> (nodes.Get(nodes.GetN() - 1)->GetObject<MobilityModel>());
			malicious2->SetPosition (Vector (780, 40, 10));
			malicious2->SetVelocity (Vector (0, 0, 0));

			Ptr<WaveNetDevice> device1 = DynamicCast<WaveNetDevice>(nodes.Get(nodes.GetN() - 2)->GetDevice(2));
			Ptr<WaveNetDevice> device2 = DynamicCast<WaveNetDevice>(nodes.Get(nodes.GetN() - 1)->GetDevice(2));
			Ptr<DosApp> dosApp1 = CreateObject<DosApp>(device1, 666);
			Ptr<DosApp> dosApp2 = CreateObject<DosApp>(device2, 667);

			dosApp1->SetStartTime(Seconds((4 * startMultiplicative) + 1)); // 4 * startMultiplicative because node 4 is the first node that will be in range
			dosApp1->SetStopTime(Seconds(simTime - 3));
			dosApp2->SetStartTime(Seconds((4 * startMultiplicative) + 24)); // add 30 beacuse it'll take the fastest node (4) 30 seconds to get within range
			dosApp2->SetStopTime(Seconds(simTime - 3));

			nodes.Get(nodes.GetN() - 1)->AddApplication(dosApp1);
			nodes.Get(nodes.GetN() - 2)->AddApplication(dosApp2);

			Simulator::Schedule(Seconds(1), &DosApp::spam, dosApp1, 20);
			Simulator::Schedule(Seconds(1), &DosApp::spam, dosApp2, 20);
		}
	}

	//attach to the different TraceSources for debug and results
	Config::Connect("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback (&RxBegin) );
	Config::Connect("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&RxDrop) );
	Config::Connect("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback (&RxBegin) );
	Config::Connect("/NodeList/1/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&RxDrop) );
	Config::Connect("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback (&RxBegin) );
	Config::Connect("/NodeList/2/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback (&RxDrop) );

	Config::Connect("/NodeList/*/DeviceList/2/$ns3::WaveNetDevice/PhyEntities/*/PhyRxBegin", MakeCallback (&WaveRxBegin) );
	Config::Connect("/NodeList/*/DeviceList/2/$ns3::WaveNetDevice/PhyEntities/*/PhyRxEnd", MakeCallback (&WaveRxEnd) );
	Config::Connect("/NodeList/*/DeviceList/2/$ns3::WaveNetDevice/PhyEntities/*/PhyRxDrop", MakeCallback (&WaveRxDrop) );
	Config::Connect("/NodeList/*/DeviceList/2/$ns3::WaveNetDevice/PhyEntities/*/PhyTxBegin", MakeCallback (&WaveTxBegin) );
	Config::Connect("/NodeList/*/DeviceList/2/$ns3::WaveNetDevice/PhyEntities/*/PhyTxEnd", MakeCallback (&WaveTxEnd) );

	Simulator::Stop(Seconds(simTime));
	Simulator::Run();

	//debug information on dropped and transmitted packets
	std::cout << BOLD_CODE << YELLOW_CODE << "dropped this many packets: " << countDrop << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "received this many packets: " << countReceived << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "wave dropped: " << waveDrop << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "wave received: " << waveReceived << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "wave received: " << waveRecvEnd << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "wave transmit start: " << waveTransmit << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "wave transEnd end: " << waveTransEnd << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "sim time: " << simTime << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "filename: " << filename << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "vehicleNum: " << vehicleNum << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "maliciousNum: " << maliciousNum << std::endl;
	std::cout << BOLD_CODE << YELLOW_CODE << "startMultiplicative: " << startMultiplicative << std::endl;

	Simulator::Destroy();
	return 0;
}//namespace ns3
