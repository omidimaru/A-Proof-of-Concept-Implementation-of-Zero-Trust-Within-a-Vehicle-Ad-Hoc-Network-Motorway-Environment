#ifndef RSU_APPLICATION_H
#define RSU_APPLICATION_H

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
#include "ns3/tag.h"

#include "AuthHeader.h"

#include <vector>
#include <string>
#include <map>

namespace ns3{
	class RsuApp : public ns3::Application{
		public:
			static TypeId GetTypeId (void);
			virtual TypeId GetInstanceTypeId (void) const;

			RsuApp();
			RsuApp(Ptr<Socket> socket, Ptr<NetDevice> device, Ipv4InterfaceContainer ipv4Container, uint32_t serialNum);
			~RsuApp();
			// ~ is a bitwise NOT operand in C++ : used above for making a class destructor

			void sendPacket(Ptr<Packet> packetSend, Address targetAddress, InetSocketAddress inet, Time broadcastRate);
			void receivePacket(Ptr<Socket> socket);
			void reply(Ptr<Packet> packet, uint16_t registered);
			void trustRequestReply(Ptr<Packet> packet, AuthHeader authHead, uint32_t sourceNode);

			//void PromiscRx(Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu, SignalNoiseDbm noise);
			void phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason);

			uint32_t getSerialNo();

			std::map<uint32_t, bool> getRecV();
			void setUnrecV(uint32_t serialNum);
			std::map<uint32_t, bool> getUnrecV();

		private:
			//inherited function from Application, code that gets executed once the application starts
			void StartApplication();

			uint32_t m_serialNo;
			Ptr<Socket> m_socket;
			Ptr<NetDevice> m_device;
			Ptr<Node> m_node;
			Ipv4InterfaceContainer m_ipv4Container;
			std::map<uint32_t, bool> m_recognised;
			std::map<uint32_t, bool> m_unrecognised;
	};

}//namespace ns3
#endif
