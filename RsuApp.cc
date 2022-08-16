#include "ns3/config-store.h"
#include "ns3/ipv4.h"
#include "ns3/ethernet-header.h"
#include "ns3/simulator.h"

#include "TargetHeader.h"
#include "SignatureHeader.h"
#include "RsuApp.h"

#include <fstream>
#include <iostream>

#define YELLOW_CODE "\033[33m"
#define RED_CODE "\033[91m"
#define GREEN_CODE "\033[32m"
#define TEAL_CODE "\033[36m"
#define BOLD_CODE "\033[1m"
#define END_CODE "\033[0m"

namespace ns3{
	NS_LOG_COMPONENT_DEFINE("RsuApp");
	NS_OBJECT_ENSURE_REGISTERED(RsuApp);

	TypeId RsuApp::GetTypeId(){
		static TypeId tid = TypeId("ns3::RsuApp")
			.SetParent<Application> ()
			.AddConstructor<RsuApp> ()
		;
		return tid;
	}

	TypeId RsuApp::GetInstanceTypeId() const{
		return RsuApp::GetTypeId();
	}

	RsuApp::RsuApp(){}

	//added a serial number to be read in and assigned to the car via this app
	//gets taken from /home/ben/code/python/carSerials or rsuSerials
	RsuApp::RsuApp(Ptr<Socket> socket, Ptr<NetDevice> device, Ipv4InterfaceContainer ipv4Container, uint32_t serialNo){
		m_socket = socket;
		m_device = device;
		m_node = socket->GetNode();
		m_serialNo = serialNo;
		m_ipv4Container = ipv4Container;

		//populate the internal map of recognised vehicles
		std::ifstream carFile ("scratch/project/carSerials.txt");
		while(!carFile.eof()){
			std::string data;
			carFile >> data;
			uint32_t serialUint = std::stoul(data);
			m_recognised[serialUint] = false;
		}

		m_socket->SetRecvCallback(MakeCallback(&RsuApp::receivePacket, this));
		m_socket->Listen();
		//std::cout << "rsu buffer size: " << DynamicCast<UdpSocket>(m_socket)->GetRcvBufSize() << std::endl;
	}

	RsuApp::~RsuApp(){}

	void RsuApp::sendPacket(Ptr<Packet> packetSend, Address targetAddress, InetSocketAddress senderAddress, Time broadcastRate){
		Ptr<Packet> packet = packetSend->Copy();

		// std::cout << packet->ToString() << std::endl;
		TargetHeader t;
		if(packet->PeekHeader(t)){
			NS_LOG_INFO("********************************RSU APP*************************************\n"
						<< "nodeId inside sendPacket: " << t.getNodeId() << "; data inside sendPacket: " << t.getMessageType()
						<< " serialNo inside sendPacket: " << t.getSerial() << "\n"
						);
		}

		m_socket->Connect(targetAddress);
		m_socket->Send(packetSend);
		Simulator::Schedule(broadcastRate, &RsuApp::sendPacket, this, packetSend, targetAddress, senderAddress, broadcastRate);
	}

	void RsuApp::receivePacket(Ptr<Socket> socket){
		while (Ptr<Packet> receivedPacket = m_socket->Recv()){
			NS_LOG_DEBUG("Inside the rsu_receive_packet function");
			//std::cout << TEAL_CODE << "Packet size: " << receivedPacket->GetSize() << END_CODE << std::endl;
			NS_LOG_INFO("****************************************************************************");
			Ptr<Packet> receivedPacketCopy = receivedPacket->Copy();

			NS_LOG_DEBUG("rsu receive packet - " << receivedPacket->ToString());

			TargetHeader checkTargetHeader;
			receivedPacket->PeekHeader(checkTargetHeader);
			if(checkTargetHeader.getMessageType() != 42){
				NS_LOG_DEBUG("type id: " << checkTargetHeader.GetTypeId());
				NS_LOG_INFO("rsu - header data: " << checkTargetHeader.getMessageType() << " header nodeId: " << checkTargetHeader.getNodeId());
				uint32_t vehicleSerial = checkTargetHeader.getSerial();
				if(checkTargetHeader.getMessageType() == 18){
					NS_LOG_DEBUG("IM IN THE RSU TEST HEADER CHECK FOR EIGHTEEN" << checkTargetHeader.getNodeId() << "\n"
								<< "I am node " << GetNode()->GetId() << " and I just received a packet from node " << checkTargetHeader.getNodeId()
								);
					uint16_t recognisedSerial = 0;

					if(m_recognised.count(vehicleSerial)){
						m_recognised[vehicleSerial] = true;
						recognisedSerial = 1;
						Simulator::ScheduleNow(&RsuApp::reply, this, receivedPacketCopy, recognisedSerial);
						NS_LOG_DEBUG("SETTING THE CAR'S SERIAL IN REGISTERED MAP");
					}else{
						m_unrecognised[vehicleSerial] = true;
						Simulator::ScheduleNow(&RsuApp::reply, this, receivedPacketCopy, recognisedSerial);
						NS_LOG_DEBUG("SETTING THE CAR'S SERIAL IN UN-REGISTERED MAP");
					}
				}
			}else{
				AuthHeader authHead;
				receivedPacket->PeekHeader(authHead);
				uint32_t sourceNode = authHead.getNodeId();
				uint32_t requestedSerial = authHead.getRequestedSerial();
				uint16_t authCode = 0;

				NS_LOG_DEBUG("rsuApp auth stuff - node: " << sourceNode << " packet: " << receivedPacket->ToString());

				if(m_recognised.count(requestedSerial)){
					//recognised serial and previously registered : recognised serial and not recently within rsu zone
					authCode = (m_recognised[requestedSerial]) ? 11 : 10;
					NS_LOG_DEBUG("auth code set to: " << authCode);
				}else if(m_unrecognised.count(requestedSerial)){
					//unrecognised serial and previously registered : unrecognised serial and not recently within rsu zone
					authCode = (m_unrecognised[requestedSerial]) ? 21 : 20;
					NS_LOG_DEBUG("auth code set to: " << authCode);
				}

				AuthHeader replyAuth = AuthHeader(uint16_t(43), authCode, requestedSerial, m_node->GetId());
				trustRequestReply(receivedPacket, replyAuth, sourceNode);
			}
		}
	}

	void RsuApp::reply(Ptr<Packet> packet, uint16_t recognisedSerial){
		TargetHeader replyTargetHeader = TargetHeader(uint32_t (27), GetNode()->GetId(), m_serialNo);
		TargetHeader checkTargetHeader;
		packet->PeekHeader(checkTargetHeader);

		Ptr<Packet> replyTestPacket = Create <Packet> (200);
		SignatureHeader signature = SignatureHeader(m_serialNo, checkTargetHeader.getSerial(), recognisedSerial);

		replyTestPacket->AddHeader(signature);
		replyTestPacket->AddHeader(replyTargetHeader);

		uint32_t sourceNode = checkTargetHeader.getNodeId();

		//Simulator::Schedule(Seconds(1.0), &RsuApp::sendPacket, this, replyTestPacket, targetAddress, inet, packetsLeft)
		InetSocketAddress toVehicle = InetSocketAddress(m_ipv4Container.GetAddress(sourceNode), 80);
		m_socket->Connect(toVehicle);
		m_socket->Send(replyTestPacket);
	}

	void RsuApp::trustRequestReply(Ptr<Packet> packet, AuthHeader authHead, uint32_t sourceNode){
		NS_LOG_DEBUG("in trust request reply");
		AuthHeader checkAuthHead;
		packet->PeekHeader(checkAuthHead);

		Ptr<Packet> replyAuth = Create <Packet> (200);

		replyAuth->AddHeader(authHead);

		//Simulator::Schedule(Seconds(1.0), &RsuApp::sendPacket, this, replyTestPacket, targetAddress, inet, packetsLeft)
		InetSocketAddress toVehicle = InetSocketAddress(m_ipv4Container.GetAddress(sourceNode), 80);
		m_socket->Connect(toVehicle);
		m_socket->Send(replyAuth);
	}


	void RsuApp::phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
	{
	  NS_LOG_INFO(Simulator::Now().GetSeconds() << RED_CODE << " Node " << GetNode()->GetId() << " RxDrop. " << END_CODE << "Reason: " << reason << std::endl);
	}


	void RsuApp::StartApplication(){
		m_device->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&RsuApp::phyRxDropTrace, this));
	}


	std::map<uint32_t, bool> RsuApp::getRecV(){
		return m_recognised;
	}


	uint32_t RsuApp::getSerialNo(){
		return m_serialNo;
	}
}//namespace ns3
