#ifndef VEHICLE_APPLICATION_H
#define VEHICLE_APPLICATION_H

#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"
#include "ns3/socket.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-interface-container.h"

#include "WaveApp.h"


#include <vector>
#include <string>

namespace ns3{
	typedef struct{
		uint32_t rsuSerial;
		Time lastContact;
		bool connected;
	} RsuConnection;

	class VehicleApp : public ns3::Application{
		public:
			static TypeId GetTypeId (void);
			virtual TypeId GetInstanceTypeId (void) const;

			VehicleApp();
			VehicleApp(Ptr<Socket> socket, Ptr<NetDevice> netDevice, Ptr<WaveNetDevice> waveDevice, Ipv4InterfaceContainer ipv4Container, uint32_t serialNo);
			~VehicleApp();
			// ~ is a bitwise NOT operand in C++ : used above for making a class destructor

			uint32_t getConnectedRsuSerial();
			void updateConnectedRsu(uint32_t newRsu);
			void checkLastContact(double interval);


			void sendPacket(Ptr<Packet> packetSend, Address targetAddress, InetSocketAddress inet);
			void receivePacket(Ptr<Socket> socket);
			void reply(Ptr<Packet> packet);
			void checkUnsureSerial(uint32_t unsureSerial);

			void phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason);
			//void PromiscRx(Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu, SignalNoiseDbm noise);

			uint32_t getSerialNo();
			uint16_t getConnectedRsuNode();

			void setConnectedRsuNode(uint16_t nodeId);

			std::map<uint32_t, RsuConnection> getRsuMap();




		private:
			//inherited function from Application, code that gets executed once the application starts
			void StartApplication();
			void StopApplication();

			void tidyTrustCalc(uint32_t unsureSerial);

			Ptr<WaveApp> m_waveApp;
			Ptr<WaveNetDevice> m_waveDevice;

			uint32_t m_serialNo;
			Ptr<Socket> m_socket;
			Ptr<NetDevice> m_netDevice;
			Ptr<Node> m_node;
			uint32_t m_nodeId;
			Ipv4InterfaceContainer m_ipv4Container;
			bool m_officialRsu;
			std::list <RsuConnection> m_rsuConnections;
			std::map<uint32_t, RsuConnection> m_rsuMap;

			//purely for ease of use within checkUnsureSerial
			uint16_t m_connectedRsuNode;

			Time m_registrationRtt;
	};

}//namespace ns3
#endif
