#include "DosApp.h"
#include "DelayTag.h"

#include <fstream>
#include <iostream>
#include <stdlib.h>

namespace ns3{
	NS_LOG_COMPONENT_DEFINE("DosApp");
	NS_OBJECT_ENSURE_REGISTERED(DosApp);

	TypeId DosApp::GetTypeId(){
		static TypeId tid = TypeId("ns3::DosApp")
			.SetParent<Application> ()
			.AddConstructor<DosApp> ()
		;
		return tid;
	}

	TypeId DosApp::GetInstanceTypeId() const{
		return DosApp::GetTypeId();
	}

	DosApp::DosApp(){}

	DosApp::DosApp(Ptr<WaveNetDevice> device, uint32_t serialNo){
		m_device = device;
		m_serialNo = serialNo;
		m_sentPackets = 0;
	}

	DosApp::~DosApp(){}

	void DosApp::spam(double broadcastRate){

		Ptr<Packet> malPacket = Create <Packet>(1501);
		SignatureHeader malSig = SignatureHeader(uint32_t(1), m_serialNo, uint16_t(1));
		malPacket->AddHeader(malSig);

		TxInfo txInfo;
		txInfo.channelNumber = SCH1;
		txInfo.preamble = WIFI_PREAMBLE_LONG;
		txInfo.priority = 7; //DoS would probably take place on the highest prority to affect the network more
		txInfo.txPowerLevel = 8;
		txInfo.dataRate = WifiMode("OfdmRate6MbpsBW10MHz");

		DelayTag dTag = DelayTag(m_serialNo, Now().GetMilliSeconds());
		malPacket->AddPacketTag(dTag);

		//double jitter = double((rand() % 800 + 1) + 100);
		m_device->SendX(malPacket, Mac48Address::GetBroadcast(), 0x88dc, txInfo);
		Simulator::Schedule(MilliSeconds(broadcastRate), &DosApp::spam, this, 1);
	}

	uint32_t DosApp::getSerialNo(){
		return m_serialNo;
	}

	void DosApp::phyTxEnd(Ptr<const Packet> packet){
		m_sentPackets++;
		//std::ofstream outfile;
		//outfile.open("results/waveSent.txt", std::ios_base::app);
		//outfile << "DoS Packet UID: " <<  packet->GetUid() << "\n";
		//outfile.close();
	}

	void DosApp::StartApplication(){
		SchInfo schInfo = SchInfo (SCH1, false, EXTENDED_ALTERNATING);
		Simulator::ScheduleNow(&WaveNetDevice::StartSch, m_device, schInfo);
		Ptr<WifiPhy> phy = m_device->GetPhys()[0];
		phy->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&DosApp::phyTxEnd, this));
		std::cout << "dos starting at " << Now().GetSeconds() << std::endl;
		//m_device->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback(&DosApp::phyRxDropTrace, this));
	}

	void DosApp::StopApplication(){
		std::cout << "DosApp sent: " << m_sentPackets << std::endl;
	}
}//namespace ns3
