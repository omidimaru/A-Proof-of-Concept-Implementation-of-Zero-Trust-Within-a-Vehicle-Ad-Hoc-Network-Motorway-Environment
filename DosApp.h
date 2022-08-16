#ifndef DOS_APPLICATION_H
#define DOS_APPLICATION_H

#include "ns3/core-module.h"
#include "ns3/wave-module.h"
#include "ns3/network-module.h"
#include "ns3/application.h"
#include "ns3/wave-net-device.h"
#include "ns3/wifi-phy.h"

#include "SignatureHeader.h"

#include <string>

namespace ns3{
	class DosApp : public ns3::Application{
		public:
			static TypeId GetTypeId (void);
			virtual TypeId GetInstanceTypeId (void) const;

			DosApp();
			DosApp(Ptr<WaveNetDevice> device, uint32_t serialNum);
			~DosApp();
			// ~ is a bitwise NOT operand in C++ : used above for making a class destructor

			void spam(double interval);
			void phyTxEnd(Ptr<const Packet> packet);
			uint32_t getSerialNo();

		private:
			//inherited function from Application, code that gets executed once the application starts
			void StartApplication();
			void StopApplication();

			uint32_t m_serialNo;
			Ptr<WaveNetDevice> m_device;
			uint64_t m_sentPackets;
	};

}//namespace ns3
#endif
