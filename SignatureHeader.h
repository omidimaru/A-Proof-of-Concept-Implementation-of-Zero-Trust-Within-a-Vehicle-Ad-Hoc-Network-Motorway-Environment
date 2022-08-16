#ifndef SIGNATURE_HEADER_H
#define SIGNATURE_HEADER_H

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/node.h"
#include <iostream>
#include <string>

namespace ns3{
    class SignatureHeader : public Header{
        public:

            SignatureHeader();
            SignatureHeader(uint32_t rsuSerial, uint32_t vehiclesSerial);
            SignatureHeader(uint32_t rsuSerial, uint32_t vehiclesSerial, uint16_t registered);
            virtual ~SignatureHeader();

            static TypeId GetTypeId(void);
            virtual TypeId GetInstanceTypeId(void) const;
            virtual uint32_t GetSerializedSize(void) const;
            virtual void Serialize(Buffer::Iterator start) const;
            virtual uint32_t Deserialize(Buffer::Iterator start);
            virtual void Print(std::ostream &os) const;

            uint16_t getRegistered();
            uint32_t getRsuSerial();
        	uint32_t getVehicleSerial();
        	uint64_t getTimestamp ();

            void setRegistered(uint16_t registered);
        	void setRsuSerial(uint32_t rsuSerial);
        	void setVehicleSerial(uint32_t vehiclesSerial);
        	void setTimestamp (uint64_t t);

        private:
            uint16_t m_registered;
            uint32_t m_rsuSerial;
            uint32_t m_vehicleSerial;
            uint64_t m_timestamp;

    };
}//namespace ns3
#endif
