#ifndef WAVE_APPLICATION_H
#define WAVE_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/wave-module.h"
#include "ns3/network-module.h"
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"

#include "SignatureHeader.h"
#include "RsuApp.h"

#include "DelayTag.h"

#include <string>
#include <tuple>
#include <utility>

namespace ns3{
	typedef struct{
		uint32_t rsuSerial;
		uint32_t vehicleSerial;
		bool registered;
		uint64_t timestamp;
	} Signature;

	typedef struct{
		bool trusted;
		double positiveEvents;
		double negativeEvents;
		//belief - disbelief - uncertainty
		std::tuple<double, double, double> trustValue;
		Signature signature;
	} TrustStruct;

	class WaveApp : public ns3::Application{
		public:
			static TypeId GetTypeId (void);
			virtual TypeId GetInstanceTypeId (void) const;

			WaveApp();
			WaveApp(Ptr<NetDevice> waveDevice, uint32_t serialNo, NodeContainer m_nodes, uint32_t malNum, std::string filename);
			~WaveApp();
			// ~ is a bitwise NOT operand in C++ : used above for making a class destructor


			void sendPacket(double interval);
			bool receivePacket(Ptr<NetDevice> device,Ptr<const Packet> packet,uint16_t protocol, const Address &sender);
			void reply(uint32_t vehicleSerial, Ptr<Packet> packetCopy);

			uint32_t getSerialNo();
			uint32_t getConnectedRsu();
			bool getSch1Enabled();
			bool getEnable();
			Signature getMySig();

			void trustHalfLife(double interval);

			void setSerialNo(uint32_t serial);
			void setSch1Enabled(bool value);
			void setEnable(bool value);
			void setMySig(SignatureHeader sig);

			void phyRxDropTrace(Ptr<const Packet> packet, WifiPhyRxfailureReason reason);
			void phyRxBegin(Ptr< const Packet > packet, RxPowerWattPerChannelBand rxPowersW);
			void phyTxEnd(Ptr<const Packet> packet);
			void phyRxEnd(Ptr< const Packet >);

			void initialiseTrust(SignatureHeader sig);
			void updateTrust(SignatureHeader sig, double pos, double neg);

			void enableSch1();
			void disableSch1();

			void updateSignatures(SignatureHeader receivedSig);

			void didTheyReply(uint32_t targetSerial);

			//void PromiscRx(Ptr<const Packet> packet, uint16_t channelFreq, WifiTxVector tx, MpduInfo mpdu, SignalNoiseDbm noise);
			std::list<uint32_t> m_connectedRsu;

			void increasePositive(uint32_t vehicle, double amount);
			void increaseNegative(uint32_t vehicle, double amount);
			void setRegistrationTime(double regTime);

			std::map<uint32_t, TrustStruct> getGoodVehicles();
			std::map<uint32_t, int64_t> getBadVehicles();
			std::map<uint32_t, Signature> getSignatures();

		private:
			void StartApplication();
			void StopApplication();
			uint32_t m_serialNo;
			Ptr<WaveNetDevice> m_device;
			uint32_t m_rsuZone;
			bool m_sch1Enabled;
			const SchInfo m_cchInfo;
			const SchInfo m_sch1Info;

			std::map<uint32_t, TrustStruct> m_goodVehicles;
			std::map<uint32_t, int64_t> m_badVehicles;
			std::map<uint32_t, Signature> m_signatures;

			NodeContainer m_nodes;
			uint32_t m_nodeId;

			bool m_enable;
			Signature m_mySig;
			//map to keep track of vehicles expected to reply - {vehicle's serial, simulator time in seconds}
			std::map<uint32_t, double> m_expectReply;
			//map to keep track of last contact between nodes
			std::map<uint32_t, double> m_lastContact;
			std::map<uint32_t, int64_t> m_firstContact;

			uint32_t malInSim;

			double m_msDelay;
			double m_throughput;
			uint64_t m_totalRxBytes;
			uint64_t m_numPktRecv;
			uint64_t m_totalGoodput;
			uint64_t m_goodput;
			uint64_t m_malGoodput;

			uint64_t testpkt;
			uint64_t recvpktbegin;

			uint64_t m_sentPackets;

			double dosDelay;
			uint32_t dospkts;
			double normDelay;
			uint32_t normpkts;

			std::string m_filename;



			Time m_activeTime;
			double m_msRegDelay;
	};

}//namespace ns3
#endif
