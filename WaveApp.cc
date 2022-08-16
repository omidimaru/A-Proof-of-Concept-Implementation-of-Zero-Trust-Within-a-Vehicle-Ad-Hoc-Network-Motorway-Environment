#include "ns3/simulator.h"
#include "WaveApp.h"
#include "VehicleApp.h"
#include "TargetHeader.h"
#include "WaveReplyHeader.h"
#include "AuthHeader.h"

#include "ns3/seq-ts-header.h"

#include <stdlib.h>
#include <chrono>

#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define PURPLE_CODE "\033[35m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

namespace ns3{
	NS_LOG_COMPONENT_DEFINE("WaveApp");
	NS_OBJECT_ENSURE_REGISTERED(WaveApp);

	Ptr<VehicleApp> getVehicleApp(Ptr<Node> node){
		return DynamicCast<VehicleApp>(node->GetApplication(0));
	}

	TypeId WaveApp::GetTypeId(){
		static TypeId tid = TypeId("ns3::WaveApp")
			.SetParent<Application> ()
			.AddConstructor<WaveApp> ()
		;
		return tid;
	}

	TypeId WaveApp::GetInstanceTypeId() const{
		return WaveApp::GetTypeId();
	}

	WaveApp::WaveApp(){}

	//added a serial number to be read in and assigned to the car via this app
	//gets taken from /home/ben/code/python/carSerials or rsuSerials
	WaveApp::WaveApp(Ptr<NetDevice> waveDevice, uint32_t serial, NodeContainer nodes, uint32_t malNum, std::string filename){
		m_device = DynamicCast<WaveNetDevice> (waveDevice);
		m_nodeId = 0;
		m_rsuZone = 0;
		m_sch1Enabled = false;
		m_enable = false;
		m_serialNo = serial;
		m_nodes = nodes;
		const SchInfo m_cchInfo = SchInfo (CCH, false, EXTENDED_ALTERNATING);
		const SchInfo m_sch1Info = SchInfo (SCH1, false, EXTENDED_ALTERNATING);
		m_msDelay = 0;
		m_numPktRecv = 0;
		m_totalRxBytes = 0;
		m_throughput = 0;
		m_totalGoodput = 0;
		m_goodput = 0;
		m_malGoodput = 0;
		m_msRegDelay = 0; //just to initialise the variable to allow for accurate results later
		malInSim = malNum; //number of malicious nodes in sim
		m_sentPackets = 0;

		dosDelay = 0;
		normDelay = 0;
		dospkts = 0;
		normpkts = 0;

		m_filename = filename;

		testpkt = 0;
		recvpktbegin = 0;
	}

	WaveApp::~WaveApp(){}

	void WaveApp::sendPacket(double interval){
		if(m_enable){
			SignatureHeader sendSig;

			sendSig.setRegistered(m_mySig.registered);
			sendSig.setRsuSerial(m_mySig.rsuSerial);
			sendSig.setVehicleSerial(m_mySig.vehicleSerial);
			sendSig.setTimestamp(m_mySig.timestamp);

			int random = rand();

			TxInfo tx;
			//if the vehicle has entered the range of the RSU, and thus the SCH1 variable has been set to true, communicate over SCH1 instead
			//178 is CCH - 172 is SCH1
			tx.channelNumber = (m_sch1Enabled) ? SCH1 : CCH;
			tx.preamble = WIFI_PREAMBLE_LONG;
			tx.priority = (random % 7) + 1; //highest priority is 7
			tx.txPowerLevel = 8;
			tx.dataRate = WifiMode("OfdmRate6MbpsBW10MHz");

			Ptr<Packet> packet = (m_sch1Enabled) ? Create <Packet> (1000) : Create<Packet>(200);

			if((random % 100) >= 95) {
				//making a tag to point the "targetted" packet at the next door neighbour
				//	every second node is on a different speed/axis so it needs to be +2/-2 to skip the immediate node neighbour
				//	minus to avoid null point exception
				NS_LOG_DEBUG("measure percent");
				if(m_nodes.GetN() - malInSim > (m_nodeId + 2)){
					uint32_t targetSerial = DynamicCast<VehicleApp>(m_nodes.Get(m_nodeId + 2)->GetApplication(0))->getSerialNo();
					WaveReplyHeader wrh = WaveReplyHeader(targetSerial, 1, 1);
					packet->AddHeader(wrh);
					Simulator::Schedule(Seconds(5), &WaveApp::didTheyReply, this, targetSerial);
					//std::cout << "sending a packet and I want a reply" << std::endl;
				}else{
					uint32_t targetSerial = DynamicCast<VehicleApp>(m_nodes.Get(m_nodeId - 2)->GetApplication(0))->getSerialNo();
					WaveReplyHeader wrh = WaveReplyHeader(targetSerial, 1, 1);
					packet->AddHeader(wrh);
					Simulator::Schedule(Seconds(5), &WaveApp::didTheyReply, this, targetSerial);
					//std::cout << "sending a packet and I want a reply" << std::endl;
				}
			}

			packet->AddHeader(sendSig);

			//0x88dc is the WSMP protocol : packet - target address - protocol - transmission info
			//std::cout << "WAVE PACKET SEND Node " << m_nodeId << " sending out packet on " << tx.channelNumber << " at " << Now().GetSeconds() << " " << packet->ToString() << std::endl;
			//std::cout << "debug wave send" << std::endl;
			//double testMe = double((rand() % 9 + 1));
			double testMe = double((rand() % 900 + 1) + 100);//get a random value between 100 - 1000 microseconds : used as packet jitter to help the vehicle more evenly broadcast packets
			Time tagTime = Now() + MicroSeconds(testMe);
			DelayTag dTag = DelayTag(m_serialNo, tagTime.GetMilliSeconds());
			packet->AddPacketTag(dTag);
			Simulator::Schedule(MicroSeconds(testMe), &WaveNetDevice::SendX, m_device, packet, Mac48Address::GetBroadcast(), 0x88dc, tx);

			//double newInterval = double((rand() % 50 + 1) + 103);
			//double newIntervalTest = double((rand() % 100 + 1) + 100); //get a random number between 50 and 100
			Simulator::Schedule(MilliSeconds(interval), &WaveApp::sendPacket, this, 100);
		}
	}

	bool WaveApp::receivePacket(Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol, const Address &sender){
		++recvpktbegin;
		//std::cout << "wave receive function node " << m_nodeId << std::endl;
		bool receive = false;
		uint16_t positive = 0;
		uint16_t negative = 0;
		Ptr<Packet> packetCopy = packet->Copy();
		if(m_enable){
			++testpkt;
			NS_LOG_INFO("********************WAVE RECEIVE********************\n"
						<< "From: " << sender << " - "
						<< packetCopy->ToString()
						);
			SignatureHeader rSig;
			WaveReplyHeader replyHeader;
			uint32_t packetSize = packet->GetSize();
			m_totalGoodput += packetSize;
			if(packetCopy->PeekHeader(rSig) && !m_badVehicles.count(rSig.getVehicleSerial())){
				m_goodput += packetSize;
				if(!m_goodVehicles.count(rSig.getVehicleSerial())){
					//std::cout << "initialising trust" << std::endl;
					initialiseTrust(rSig);
				}
				uint32_t vehicleSerial = rSig.getVehicleSerial();
				//if(vehicleSerial == 999999 && m_nodeId == 4){
					//std::cout << Now().GetSeconds() << " is the time" << std::endl;
			//	}
				uint32_t rsuSerial = rSig.getRsuSerial();
				Ptr<VehicleApp> vApp = DynamicCast<VehicleApp>(GetNode()->GetApplication(0));

				//std::cout << "debug goop" << std::endl;
				updateSignatures(rSig);

				receive = true;
				//if the signature is not in this RSU zone or the last
				if(m_signatures[vehicleSerial].rsuSerial != m_connectedRsu.front() && m_signatures[vehicleSerial].rsuSerial != m_connectedRsu.back()){
					//maybe the vehicle just came from a different route - don't weight it heavily
					NS_LOG_DEBUG("signature not in rsu zone");
					++negative;
				}else{
					++positive;
				}

				if(packetCopy->GetSize() > 1500){
					NS_LOG_DEBUG("packet bigger than 1500");
					negative += 2;
				}

				//if the RSU from which the signature originates does not exist/is not known
				if(!vApp->getRsuMap().count(rsuSerial)){
					//all vehicles should have a comprehensive list of RSUs - to see a signature with an unknown RSU would be unusual - weigh it slightly more
					NS_LOG_DEBUG("rsu unrecognised" << std::endl);
					negative += 2;
				}else{
					++positive;
				}

				Time sixtysAgo = (Now() - Seconds(60));
				//if the timestamp has been forged to be from the future or from too far in the past
				if(m_signatures[vehicleSerial].timestamp > uint64_t(Simulator::Now().GetTimeStep()) || int64_t(m_signatures[vehicleSerial].timestamp) < sixtysAgo.GetTimeStep()) {
					//for a timestamp to be unsynced from the rest of the network such that it seems to be coming from the future
					//or too far in the past - weigh it heavily negative
					NS_LOG_DEBUG("timestamp old");
					negative += 3;
				}else{
					++positive;
				}

				if(m_lastContact.count(vehicleSerial) && (Now().GetSeconds() < (m_lastContact[vehicleSerial] + 0.05))){
					std::cout << "packets coming too quickly" << std::endl;
					negative += 4;
				}

				NS_LOG_INFO("this is the trust value of the current packet coming in: " << std::get<2>(m_goodVehicles[vehicleSerial].trustValue));
				if(std::get<2>(m_goodVehicles[vehicleSerial].trustValue) > 0.5){
					NS_LOG_DEBUG("checking unsure serial from wave app");
					vApp->checkUnsureSerial(vehicleSerial);
				}

				//make sure the packet tag is directed at me before replying - else ignore and update trust normally
				packetCopy->RemoveHeader(rSig);
				if(packetCopy->PeekHeader(replyHeader)){
					//is the packet directed at me and expects a reply
					NS_LOG_DEBUG("checking the reply header: " << packetCopy->ToString());
					if(replyHeader.getTarget() == m_serialNo && (replyHeader.getExpectReply())){
						//need to send the original packets sender to reply because the signature header has been removed already
						//std::cout << "im the target and I need to reply" << std::endl;
						reply(vehicleSerial, packetCopy);
					}else if(replyHeader.getTarget() == m_serialNo && !(replyHeader.getExpectReply())){
						//std::cout << "received a response back, erasing, and adding to positive" << std::endl;
						m_expectReply.erase(vehicleSerial);
						++positive;
					}
				}
			}else{
				NS_LOG_INFO(BOLD_CODE << PURPLE_CODE << "Received either a malformed packet or its from an untrusted vehicle - within node: "
								<< m_nodeId << END_CODE);
				//if(m_badVehicles.count(rSig.getVehicleSerial())){
				m_malGoodput += packetSize;
				return true;
				//}
				//++negative;
			}
			//std::cout << "positives/negatives before trust update: " << positive << "/" << negative << std::endl;
			NS_LOG_DEBUG("updating trust in receive");
			updateTrust(rSig, positive, negative);
			m_lastContact[rSig.getVehicleSerial()] = Now().GetSeconds();
			if(!m_firstContact.count(rSig.getVehicleSerial())){
				m_firstContact[rSig.getVehicleSerial()] = Now().GetMilliSeconds();
			}
		}
		return receive;
	}

	void WaveApp::reply(uint32_t sender, Ptr<Packet> packetCopy){
		WaveReplyHeader wrh;

		packetCopy->PeekHeader(wrh);

		WaveReplyHeader replyWrh = WaveReplyHeader(sender, wrh.getSeq(), 0);

		int random = rand();
		TxInfo tx;
		//if the vehicle has entered the range of the RSU, and thus the SCH1 variable has been set to true, communicate over SCH1 instead
		//178 is CCH - 172 is SCH1
		tx.channelNumber = CCH;
		tx.preamble = WIFI_PREAMBLE_LONG;
		tx.priority = 7; //always send reply at top priority
		tx.txPowerLevel = 7;
		tx.dataRate = WifiMode("OfdmRate6MbpsBW10MHz");

		SignatureHeader sendSig;

		sendSig.setRegistered(m_mySig.registered);
		sendSig.setRsuSerial(m_mySig.rsuSerial);
		sendSig.setVehicleSerial(m_mySig.vehicleSerial);
		sendSig.setTimestamp(m_mySig.timestamp);

		Ptr<Packet> packet = Create <Packet> (200);

		packet->AddHeader(replyWrh);
		packet->AddHeader(sendSig);

		double testMe = double((random % 777 + 1) + 333); //get a random number between 333 and 1000 milliseconds
		//std::cout << "wave reply function" << std::endl;
		Simulator::Schedule(MicroSeconds(testMe), &WaveNetDevice::SendX, m_device, packet, Mac48Address::GetBroadcast(), 0x88dc, tx);
	}

	uint32_t WaveApp::getConnectedRsu(){
		return m_connectedRsu.front();
	}

	void WaveApp::initialiseTrust(SignatureHeader receivedSig){
		TrustStruct trust;
		//belief - disbelief - uncertainty
		trust.trustValue = {0.0, 0.0, 1};
		trust.positiveEvents = 0;
		trust.negativeEvents = 0;
		trust.trusted = false;
		m_goodVehicles[receivedSig.getVehicleSerial()] = trust;
		updateSignatures(receivedSig);
	}

	void WaveApp::updateTrust(SignatureHeader sig, double pos, double neg){
		uint32_t vehicle = sig.getVehicleSerial();
		bool badGuy = false;
		if(m_goodVehicles.count(vehicle) && !m_badVehicles.count(vehicle)){
			TrustStruct trust = m_goodVehicles[vehicle];
			//std::cout << "trust positive/negative events for " << vehicle << " in update trust before addition: " << trust.positiveEvents << "/" << trust.negativeEvents << std::endl;
			trust.positiveEvents += pos;
			trust.negativeEvents += neg;
			double posCalc, negCalc, uncCalc;
			posCalc = (trust.positiveEvents / (trust.positiveEvents + trust.negativeEvents + 2));
			negCalc = (trust.negativeEvents / (trust.positiveEvents + trust.negativeEvents + 2));
			uncCalc = (2 / (trust.positiveEvents + trust.negativeEvents + 2));

			//std::cout << "posCalc/negCalc/uncCalc : " << posCalc << "/" << negCalc << "/" << uncCalc << std::endl;

			trust.trustValue = {posCalc, negCalc, uncCalc};
			m_goodVehicles[vehicle] = trust;
			/*std::cout << "heres the trust in update trust: {"
				<< std::get<0>(trust.trustValue) << ", "
				<< std::get<1>(trust.trustValue) << ", "
				<< std::get<2>(trust.trustValue) << "}"
				<< std::endl;*/
			if(std::get<1>(trust.trustValue) > 0.5){
				NS_LOG_DEBUG("this is the bad guy setting");
				badGuy = true;
			}

		//must be in bad guy list
		}else{
			//don't update trust of untrusted (bad) nodes
			NS_LOG_INFO("THIS VEHICLE: " << vehicle << " IS UNTRUSTED BY NODE " << m_nodeId);
			badGuy = true;
		}
		if(badGuy){
			NS_LOG_DEBUG(vehicle << " going into bad guys");
			m_badVehicles[vehicle] = Simulator::Now().GetMilliSeconds();
			m_goodVehicles.erase(vehicle);
		}
	}

	void WaveApp::increasePositive(uint32_t vehicle, double amount){
		m_goodVehicles[vehicle].positiveEvents += amount;
		double posCalc, negCalc, uncCalc;
		posCalc = (m_goodVehicles[vehicle].positiveEvents / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		negCalc = (m_goodVehicles[vehicle].negativeEvents / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		uncCalc = (2 / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		m_goodVehicles[vehicle].trustValue = {posCalc, negCalc, uncCalc};
	}

	void WaveApp::increaseNegative(uint32_t vehicle, double amount){
		m_goodVehicles[vehicle].negativeEvents += amount;
		double posCalc, negCalc, uncCalc;
		posCalc = (m_goodVehicles[vehicle].positiveEvents / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		negCalc = (m_goodVehicles[vehicle].negativeEvents / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		uncCalc = (2 / (m_goodVehicles[vehicle].positiveEvents + m_goodVehicles[vehicle].negativeEvents + 2));
		m_goodVehicles[vehicle].trustValue = {posCalc, negCalc, uncCalc};
	}

	//create a new Signature from the SignatureHeader and use it to replace the value in the m_signatures map
	void WaveApp::updateSignatures(SignatureHeader receivedSig){
		uint32_t vSerial = receivedSig.getVehicleSerial();
		Signature sig;
		//if the vehicle's signature has not yet been seen
		if(!m_signatures.count(vSerial)){
			sig.rsuSerial = receivedSig.getRsuSerial();
			sig.vehicleSerial = receivedSig.getVehicleSerial();
			sig.registered = receivedSig.getRegistered();
			sig.timestamp = receivedSig.getTimestamp();
			m_signatures[vSerial] = sig;
		//if the vehicle's RSU zone has changed
		}else if(m_signatures[vSerial].rsuSerial != receivedSig.getRsuSerial()){
			m_signatures[vSerial].rsuSerial = receivedSig.getRsuSerial();
			m_signatures[vSerial].timestamp = receivedSig.getTimestamp();
		}
	}

	//this function is only called 5 seconds after sending the expected reply packet
	//	if the vehicle replied in that time, we will no longer expect a reply and increase the trust positive events
	//	otherwise, increase negative events and recalculate trust
	void WaveApp::didTheyReply(uint32_t targetSerial){
		NS_LOG_DEBUG("inside didTheyReply");
		if(m_expectReply.count(targetSerial)){
			std::cout << "they did not reply" << std::endl;
			TrustStruct trust = m_goodVehicles[targetSerial];
			trust.negativeEvents += 2;
			double posCalc, negCalc, uncCalc;
			posCalc = (trust.positiveEvents / (trust.positiveEvents + trust.negativeEvents + 2));
			negCalc = (trust.negativeEvents / (trust.positiveEvents + trust.negativeEvents + 2));
			uncCalc = (2 / (trust.positiveEvents + trust.negativeEvents + 2));

			trust.trustValue = {posCalc, negCalc, uncCalc};
			m_goodVehicles[targetSerial] = trust;
		}
	}

	void WaveApp::trustHalfLife(double interval){
		for(auto it = m_goodVehicles.begin(); it != m_goodVehicles.end(); ++it ){
			//first = {key} ; second = {key value} which is type TrustStruct
			//TrustStruct holds all the trust values for a particular vehicle within this node
			if(it->second.positiveEvents != 0){
				it->second.positiveEvents = floor(it->second.positiveEvents / 2);
				it->second.negativeEvents = floor(it->second.negativeEvents / 2);
			}
		}
		Simulator::Schedule(Seconds(interval), &WaveApp::trustHalfLife, this, double(15));
	}


	//setters, getters, miscellany
	void WaveApp::enableSch1(){
		m_device->StartSch(m_sch1Info);
	}

	void WaveApp::disableSch1(){
		m_device->StopSch(SCH1);
	}

	void WaveApp::setSch1Enabled(bool value){
		m_sch1Enabled = value;
		NS_LOG_DEBUG("node " << m_nodeId << " sch1Enabled set to " << value << " at " << Simulator::Now().GetSeconds());
	}

	bool WaveApp::getSch1Enabled(){
		return m_sch1Enabled;
	}

	void WaveApp::setEnable(bool value){
		m_enable = value;
		m_activeTime = Now();
	}

	bool WaveApp::getEnable(){
		return m_enable;
	}

	void WaveApp::setMySig(SignatureHeader sig){
		std::cout << "set my sig for node " << m_nodeId;
		sig.Print(std::cout);
		std::cout << std::endl;
		m_mySig.rsuSerial = sig.getRsuSerial();
		m_mySig.vehicleSerial = sig.getVehicleSerial();
		m_mySig.registered = sig.getRegistered();
		m_mySig.timestamp = sig.getTimestamp();
	}

	Signature WaveApp::getMySig(){
		return m_mySig;
	}

	uint32_t WaveApp::getSerialNo(){
		return m_serialNo;
	}





	std::map<uint32_t, TrustStruct> WaveApp::getGoodVehicles(){
		return m_goodVehicles;
	}

	std::map<uint32_t, int64_t> WaveApp::getBadVehicles(){
		return m_badVehicles;
	}

	std::map<uint32_t, Signature> WaveApp::getSignatures(){
		return m_signatures;
	}


	void WaveApp::setRegistrationTime(double regTime){
		m_msRegDelay = regTime;
	}




	void WaveApp::phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason){
		//if not dropped for being out of range, basically
		if(reason != PREAMBLE_DETECT_FAILURE && m_enable){
			//std::cout << "wave app drop" << std::endl;
			// Ptr<Packet> pCopy = packet->Copy();
			// SignatureHeader t;
			// pCopy->PeekHeader(t);
			// NS_LOG_INFO(Simulator::Now().GetSeconds() << PURPLE_CODE << " Node " << GetNode()->GetId() << " RxDrop. wavedrop " << END_CODE << "Reason: " << reason
			// 			<< " came from node: " << t.getVehicleSerial()
			// 			<< "Packet: " << packet->ToString()
			// 		);
		}
	}

	void WaveApp::phyRxEnd(Ptr< const Packet > packet){
		if(m_enable){
			DelayTag tag;
			if(packet->PeekPacketTag(tag)){
				//uint32_t senderVehicle = tag.getSender();
				//if(!m_badVehicles.count(senderVehicle)){
					//std::cout << "wave app end rx" << std::endl;
					m_totalRxBytes += packet->GetSize();
					++m_numPktRecv;
					m_msDelay += double(Now().GetMilliSeconds() - tag.getTime());

					std::ofstream outfile;
					outfile.open("results/" + m_filename + "-waveReceive.txt", std::ios_base::app);
					outfile << "Packet UID: " <<  packet->GetUid() << "\n";
					outfile.close();

					std::cout << "wave packet rx" << std::endl;

					//if the sender was a DoS node - collecting data to see if it's the DoS packets being slowed, vehicles packets, or both
					if(tag.getSender() == 666 || tag.getSender() == 667){
						dosDelay += double(Now().GetMilliSeconds() - tag.getTime());
						++dospkts;

					}else{
						normDelay += double(Now().GetMilliSeconds() - tag.getTime());
						++normpkts;
					}
				//}
			}
		}
	}

	void WaveApp::phyRxBegin(Ptr< const Packet > packet, RxPowerWattPerChannelBand rxPowersW){
		if(m_enable){
			//std::cout << "wave app rx" << std::endl;
		}
	}

	void WaveApp::phyTxEnd(Ptr<const Packet> packet){
		if(m_enable){
			m_sentPackets++;

			std::ofstream outfile;
			outfile.open("results/" + m_filename + "-waveSent.txt", std::ios_base::app);
			outfile << "Packet UID: " <<  packet->GetUid() << "\n";
			outfile.close();
		}
	}




	void WaveApp::StartApplication(){
		m_device->SetReceiveCallback(MakeCallback(&WaveApp::receivePacket, this));
		Simulator::ScheduleNow(&WaveNetDevice::StartSch, m_device, m_cchInfo);
		Simulator::Schedule(Seconds(15), &WaveApp::trustHalfLife, this, double(15));

		m_nodeId = GetNode()->GetId();

		Ptr<WifiPhy> phy = m_device->GetPhys()[0]; //default, there's only one PHY in a WaveNetDevice
		//phy->TraceConnectWithoutContext ("PhyRxBegin", MakeCallback(&WaveApp::phyRxBegin, this));
		//phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&WaveApp::phyRxDropTrace, this));
		phy->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&WaveApp::phyTxEnd, this));
		phy->TraceConnectWithoutContext("PhyRxEnd", MakeCallback(&WaveApp::phyRxEnd, this));

		NS_LOG_INFO("Node " << m_nodeId << "s wave app has started at " << Simulator::Now().GetSeconds());
	}

	void WaveApp::StopApplication(){
		m_enable = false;
		m_device->StopSch(SCH1);
		m_device->StopSch(CCH);
		NS_LOG_INFO("stopping node " << m_nodeId << " wave application at " << Now().GetTimeStep());
		//trying to see all the different pos/neg events stored in each vehicle
		std::cout << "Good vehicles for node: " << m_nodeId << std::endl;
		for(auto i : m_goodVehicles){
				std::cout << "\tgvehicle " << i.first
				<< " positive" << i.second.positiveEvents
				<< " negative" << i.second.negativeEvents
				<< std::endl;
		}
		if(m_badVehicles.size() > 0){
			std::cout << "Bad vehicles for node " << m_nodeId << std::endl;
			for(auto i : m_badVehicles){
				std::cout << "\tbvehicle " << i.first << " " << i.second << " " << m_firstContact[i.first] << "\n";
			}
			std::cout << std::endl;
		}

		std::cout << "Node " << m_nodeId << " sent: " << m_sentPackets << std::endl;



		std::cout << "Average packet delay for node " << m_nodeId << ": ";
		double delay = m_msDelay / m_numPktRecv;
		std::cout << delay << std::endl;

		std::cout << "Average throughput Bps for node " << m_nodeId << " : ";
		//double activeFor = Now().GetSeconds() - m_activeTime.GetSeconds();
		//double throughput = m_totalRxBytes;
		std::cout << m_totalRxBytes << std::endl;
		std::cout << "polywag node " << m_nodeId << " " << m_totalRxBytes << " " << m_numPktRecv << std::endl;
		std::cout << "node " << m_nodeId << " " << testpkt << " " << recvpktbegin << " " << m_numPktRecv << std::endl;

		std::cout << "Total Goodput, Trusted Goodput, Malicious Goodput - Node " << m_nodeId << " : ";
		std::cout << m_totalGoodput << " " << m_goodput << " " << m_malGoodput << std::endl;

		std::cout << "dos delay: " << dosDelay / dospkts << " " << dospkts << std::endl;
		std::cout << "norm delay: " << normDelay / normpkts << " " << normpkts << std::endl;
	}

}//namespace ns3
