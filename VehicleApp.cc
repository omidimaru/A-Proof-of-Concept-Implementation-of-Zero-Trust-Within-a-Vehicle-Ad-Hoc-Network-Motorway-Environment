#include "ns3/socket.h"
#include "ns3/config-store.h"
#include "ns3/ipv4.h"
#include "ns3/ethernet-header.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/inet-socket-address.h"
#include "ns3/mobility-module.h"

#include "TargetHeader.h"
#include "VehicleApp.h"
#include "AuthHeader.h"

#include <fstream>
#include <iostream>
#include <stdlib.h>

#define YELLOW_CODE "\033[33m"
#define TEAL_CODE "\033[36m"
#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

//this class is the main driver behind the control of everything - it sets up the connection with the RSU nodes and tells the
//vehicle's WaveApp when it can start transmitting packets
namespace ns3{
	NS_LOG_COMPONENT_DEFINE("VehicleApp");
	NS_OBJECT_ENSURE_REGISTERED(VehicleApp);

	TypeId VehicleApp::GetTypeId(){
		static TypeId tid = TypeId("ns3::VehicleApp")
			.SetParent<Application> ()
			.AddConstructor<VehicleApp> ()
		;
		return tid;
	}

	TypeId VehicleApp::GetInstanceTypeId() const{
		return VehicleApp::GetTypeId();
	}

	VehicleApp::VehicleApp(){}

	//added a serial number to be read in and assigned to the car via this app
	//gets taken from /home/ben/code/python/carSerials or rsuSerials
	VehicleApp::VehicleApp(Ptr<Socket> socket, Ptr<NetDevice> netDevice, Ptr<WaveNetDevice> waveDevice, Ipv4InterfaceContainer ipv4Container, uint32_t serialNo){
		m_socket = socket;
		m_netDevice = netDevice;
		m_waveDevice = waveDevice;
		m_node = socket->GetNode();
		m_nodeId = m_node->GetId();
		m_serialNo = serialNo;
		m_ipv4Container = ipv4Container;
		m_officialRsu = false;

		std::ifstream rsuFile ("scratch/project/rsuSerials.txt");
		int count = 0;
		while(!rsuFile.eof()){
			count++;
			NS_LOG_DEBUG("in while loop of Vehicle app - count: " << count);
			std::string data;
			rsuFile >> data;
			uint32_t serialUint = std::stoul(data);

			RsuConnection rsu;
			rsu.rsuSerial = serialUint;
			rsu.connected = false;
			m_rsuMap[serialUint] = rsu;
		}

		// making a dummy conneciton to avoid null pointer exception later when checking the connected RSU list for first time
		RsuConnection dummyConnection;
		dummyConnection.rsuSerial = 0;
		dummyConnection.connected = false;
		dummyConnection.lastContact = Simulator::Now();
		m_rsuConnections.push_front(dummyConnection);

		//just initialising the variable for later use in receivePacket
		m_registrationRtt = Now();
	}

	VehicleApp::~VehicleApp(){}

	void VehicleApp::sendPacket(Ptr<Packet> packetSend, Address targetAddress, InetSocketAddress myAddress){
		NS_LOG_DEBUG(BOLD_CODE << TEAL_CODE << "Beginning sendPacket function. This is the Sim time:"
						<< Simulator::Now() << END_CODE
					);
		Ptr<Packet> packetCopy = packetSend->Copy();
		//std::cout << packetCopy->ToString() << std::endl;
		InetSocketAddress sendTo = InetSocketAddress::ConvertFrom(targetAddress);

		//std::cout << packetCopy->ToString() << std::endl;
		TargetHeader t;
		if(packetCopy->PeekHeader(t)){
			NS_LOG_INFO("*******************************VEHICLE APP*********************************************\n"
						<< "nodeId inside sendPacket: " << t.getNodeId() << "; data inside sendPacket: " << t.getMessageType() << "\n"
						<< " serialNo inside sendPacket: " << t.getSerial()
						);
		}

		NS_LOG_DEBUG("****************************************************************************\n"
					<< "I am node: " << m_node->GetId() << " at ip: " << myAddress.GetIpv4()
					<< "I just sent a packet to: " << sendTo.GetIpv4()
					);

		m_socket->Connect(targetAddress);
		m_socket->Send(packetSend);
	}

	void VehicleApp::receivePacket(Ptr<Socket> socket){
		while (Ptr<Packet> receivedPacket = m_socket->Recv()){
			TargetHeader targetHeader;
			AuthHeader authHead;
			receivedPacket->PeekHeader(targetHeader);
			if(targetHeader.getMessageType() == 21 || targetHeader.getMessageType() == 27){
				NS_LOG_DEBUG("vehicle - header data: " << targetHeader.getMessageType() << " header nodeId: " << targetHeader.getNodeId());
				//check that the RSU exists within the vehicle's RSU map
				if(m_rsuMap.count(targetHeader.getSerial())){
					if(targetHeader.getSerial() != getConnectedRsuSerial()){
						NS_LOG_DEBUG("IM IN THE TEST HEADER CHECK FOR TWENTY ONE");
						NS_LOG_INFO("I am node " << GetNode()->GetId() << " and I just received a packet from node " << targetHeader.getNodeId());

						//21 is the original advertisement from the RSU    	<---
						//18 is set in the reply to RSU						--->
						//27 is the expected auth response from RSU			<---
						//42 is an auth request from waveApp to RSUs		--->
						//43 is the auth response from RSU to WaveApp		<---
						if(targetHeader.getMessageType() == 21 && !m_rsuMap[targetHeader.getSerial()].connected){
							NS_LOG_INFO("replying to the " << targetHeader.getSerial() << " RSU 21 from within the vehicle app of node: "
									<< GetNode()->GetId() << " at " << Simulator::Now().GetSeconds()
									);
							m_rsuMap[targetHeader.getSerial()].lastContact = Simulator::Now();
							//the received packet is used within reply to extract the target address
							this->reply(receivedPacket);
							m_registrationRtt = Now();

						}else if(targetHeader.getMessageType() == 27){
							m_rsuMap[targetHeader.getSerial()].lastContact = Simulator::Now();
							NS_LOG_DEBUG("I'VE GOT MY REPLY FROM THE RSU node: " << GetNode()->GetId() << " at " << Simulator::Now().GetSeconds());
							if(Simulator::Now().GetSeconds() > m_rsuMap[targetHeader.getSerial()].lastContact.GetSeconds() + double(3)){
								std::cout << "Too much time has passed, waiting for new advertisement packet to restart" << std::endl;
							}else{
								m_connectedRsuNode = targetHeader.getNodeId();

								//also updates the WaveApp current RSU zone
								updateConnectedRsu(targetHeader.getSerial());
								m_waveApp->setSch1Enabled(true);

								SignatureHeader signature;
								receivedPacket->RemoveHeader(targetHeader);
								receivedPacket->PeekHeader(signature);
								m_waveApp->setMySig(signature);

								//checking if the waveApp is already sending periodically to avoid having a send schedule for each RSU
								if(m_waveApp->getEnable() == false){
									//std::cout << "enabling node " << GetNode()->GetId() << " with packet signature: " << receivedPacket->ToString() << std::endl;
									m_waveApp->setEnable(true);
									//m_waveApp->enableSch1();

									//start waveApp broadcasting after 100ms - 3s at a rate of 10 packet/s
									double start = double((rand() % 2900 + 1) + 100);
									NS_LOG_DEBUG("starting the send packet of wave app " << m_node->GetId() << " at " << Simulator::Now().GetSeconds());
									Simulator::Schedule(MilliSeconds(start), &WaveApp::sendPacket, m_waveApp, (start/2.0));

									//calculate the RTT of the registration process
									double rtt = double(Now().GetMilliSeconds() - m_registrationRtt.GetMilliSeconds());
									m_waveApp->setRegistrationTime(rtt);
									std::cout << "node " << m_nodeId << " registration rtt " << rtt << std::endl;
								}
							}
						}
					}else{
						//rsu exists and has already established a connection - needed so SCH1 access can be revoked quicky when out of RSU range
						updateConnectedRsu(targetHeader.getSerial());
					}
				}
			}else if(receivedPacket->PeekHeader(authHead)){
				if(authHead.getMessageType() == 43){
					uint32_t authCode = authHead.getAuthCode();
					uint32_t unsureSerial = authHead.getRequestedSerial();
					if(m_waveApp->getGoodVehicles().count(unsureSerial)){
						switch(authCode){
							//recognised and recently in area
							case 11:
								m_waveApp->increasePositive(unsureSerial, 3.0);
								tidyTrustCalc(unsureSerial);
								break;

							//recognised and not recently in area
							case 10:
								m_waveApp->increaseNegative(unsureSerial, 2.0);
								tidyTrustCalc(unsureSerial);
								break;

							//unrecognised and recently in area
							case 21:
								m_waveApp->increasePositive(unsureSerial, 1.0);
								tidyTrustCalc(unsureSerial);
								break;

							//unrecognised and not recently in area
							case 20:
								std::cout << "not in area" << std::endl;
								m_waveApp->increaseNegative(unsureSerial, 5.0);
								tidyTrustCalc(unsureSerial);
								break;
						}
					}
				}
			}
		}
	}

	void VehicleApp::reply(Ptr<Packet> packet){
		uint32_t nodeId = m_node->GetId();
		TargetHeader targetHeader;
		packet->PeekHeader(targetHeader);

		TargetHeader replyTargetHeader = TargetHeader(uint32_t (18), nodeId, m_serialNo);
		Ptr<Packet> replyTestPacket = Create <Packet>(200);

		replyTestPacket->AddHeader(replyTargetHeader);


		InetSocketAddress myAddress = InetSocketAddress(m_ipv4Container.GetAddress(m_node->GetId()));
		InetSocketAddress targetAddress = InetSocketAddress(m_ipv4Container.GetAddress(targetHeader.getNodeId()), 80);
		NS_LOG_DEBUG(YELLOW_CODE << "I AM INSIDE THE CAR_REPLY FUNCTION AND THIS IS THE ADDRESS IM CONNECTING TO: " << targetAddress << END_CODE);

		NS_LOG_DEBUG(BOLD_CODE << TEAL_CODE << "Beginning sendPacket function in node " << GetNode()->GetId() << ". This is the Sim time:"
					<< Simulator::Now() << END_CODE << END_CODE
					);
		NS_LOG_DEBUG("reply packet in vehicle app: " << replyTestPacket->ToString());
		Simulator::ScheduleNow(&VehicleApp::sendPacket, this, replyTestPacket, targetAddress, myAddress);
	}

	void VehicleApp::checkUnsureSerial(uint32_t unsureSerial){
		uint32_t nodeId = m_node->GetId();
		// AuthHeader(uint16_t messageType, uint16_t authCode, uint32_t requestedSerial, uint32_t nodeId)
		AuthHeader authHead = AuthHeader(uint16_t(42), uint16_t(0), unsureSerial, nodeId);

		Ptr<Packet> regConfirm = Create <Packet>(200);

		regConfirm->AddHeader(authHead);

		InetSocketAddress myAddress = InetSocketAddress(m_ipv4Container.GetAddress(m_node->GetId()));
		InetSocketAddress targetAddress = InetSocketAddress(m_ipv4Container.GetAddress(m_connectedRsuNode), 80);
		//std::cout << "inside the vehicle app checkUnsureSerial - ";
		//std::cout << regConfirm->ToString() << std::endl;

		Simulator::ScheduleNow(&VehicleApp::sendPacket, this, regConfirm, targetAddress, myAddress);
	}


	uint32_t VehicleApp::getConnectedRsuSerial(){
		return m_rsuConnections.front().rsuSerial;
	}

	void VehicleApp::updateConnectedRsu(uint32_t rsuSerial){
		m_rsuMap[rsuSerial].connected = true;
		m_rsuMap[rsuSerial].lastContact = Simulator::Now();
		RsuConnection copy = m_rsuMap[rsuSerial];

		NS_LOG_DEBUG(GREEN_CODE << Simulator::Now() << " : Node " << GetNode()->GetId() << " is adding a RSU: " << rsuSerial << END_CODE);

		if(m_rsuConnections.size() > 0){
			m_rsuConnections.front().connected = false;
		}

	    m_rsuConnections.push_front(copy);
		m_waveApp->m_connectedRsu.push_front(rsuSerial);
		if(m_rsuConnections.size() > 2){
			NS_LOG_DEBUG("Node " << m_node->GetId() << " removing rsu: " << m_rsuConnections.back().rsuSerial << " from the back of the list");
			m_rsuConnections.pop_back();
			m_waveApp->m_connectedRsu.pop_back();
		}
		if(m_rsuConnections.front().rsuSerial == rsuSerial){
			m_rsuConnections.front().lastContact = Simulator::Now();
		}else{
			m_rsuConnections.back().lastContact = Simulator::Now();
		}
	}

	//setters, getters, miscellany
	void VehicleApp::setConnectedRsuNode(uint16_t nodeId){
		m_connectedRsuNode = nodeId;
	}

	uint16_t VehicleApp::getConnectedRsuNode(){
		return m_connectedRsuNode;
	}

	uint32_t VehicleApp::getSerialNo(){
		return m_serialNo;
	}

	std::map<uint32_t, RsuConnection> VehicleApp::getRsuMap(){
		return m_rsuMap;
	}

	//used to disable SCH1 access within the vehicle's WaveApp
	void VehicleApp::checkLastContact(double interval){
		if(Simulator::Now().GetSeconds() > (m_rsuConnections.front().lastContact.GetSeconds() + 0.5) && m_waveApp->getSch1Enabled()){
			m_waveApp->disableSch1();
			m_waveApp->setSch1Enabled(false);
			// m_waveDevice->StopSch(SCH1);
		}
		//keep checking
		Simulator::Schedule(Seconds(interval), &VehicleApp::checkLastContact, this, interval);
	}

	void VehicleApp::phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
	{
	  NS_LOG_INFO(Simulator::Now().GetSeconds() << RED_CODE << " VehicleNode " << GetNode()->GetId() << " RxDrop. " << END_CODE << "Reason: " << reason);
	}


	void VehicleApp::StartApplication(){
		PacketMetadata::Enable ();
		NS_LOG_FUNCTION(this);

		//set a receive callback
		m_socket->SetRecvCallback(MakeCallback(&VehicleApp::receivePacket, this));
		m_waveApp = StaticCast<WaveApp>(GetNode()->GetApplication(1));

		//using the mobility model from the node to set the node's velocity
		Ptr<ConstantVelocityMobilityModel> cvmm = DynamicCast<ConstantVelocityMobilityModel> (GetNode()->GetObject<MobilityModel>());
		//double accl = (GetNode()->GetId() % 2 == 1) ? 21 : 33;
		//double accl = 27.7; // takes roughly 46seconds for a node to go from start to leaving network at this speed - translates to roughly 100km/h
		double accl = (GetNode()->GetId() % 2 == 1) ? 21 : 33; // odd nodes take 39 seconds, even take 63 seconds - to traverse network
		//double accl = 21;
		//cvmm->SetPosition ( Vector (200, 20 + (GetNode()->GetId() % 2) * 5, 1));
		cvmm->SetPosition ( Vector (-300, 20 + (GetNode()->GetId() % 2) * 5, 1));
		cvmm->SetVelocity ( Vector (accl, 0, 0) );

		m_netDevice->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&VehicleApp::phyRxDropTrace, this));
	}

	void VehicleApp::StopApplication(){
		Ptr<ConstantVelocityMobilityModel> cvmm = DynamicCast<ConstantVelocityMobilityModel> (GetNode()->GetObject<MobilityModel>());
		//cvmm->SetPosition ( Vector (200, 20 + (GetNode()->GetId() % 2) * 5, 1));
		cvmm->SetPosition ( Vector (-300, -800, 0));
		cvmm->SetVelocity ( Vector (0, 0, 0) );
	}

	//purely used to tidy code within the AuthHeader receive packet section
	void VehicleApp::tidyTrustCalc(uint32_t unsureSerial){
		//std::cout << "updating trust within vehicle app tidy trust calc" << std::endl;
		TrustStruct trust = m_waveApp->getGoodVehicles()[unsureSerial];
		double posCalc, negCalc, uncCalc;
		posCalc = (trust.positiveEvents / (trust.positiveEvents + trust.negativeEvents + 2));
		negCalc = (trust.negativeEvents / (trust.positiveEvents + trust.negativeEvents + 2));
		uncCalc = (2 / (trust.positiveEvents + trust.negativeEvents + 2));
		std::tuple<double, double, double> trustTuple = {posCalc, negCalc, uncCalc};
		//adjust the trust tuple within the waveApp's goodVehicles map regarding the requested unsureSerial
		m_waveApp->getGoodVehicles()[unsureSerial].trustValue = trustTuple;
	}

}//namespace ns3
