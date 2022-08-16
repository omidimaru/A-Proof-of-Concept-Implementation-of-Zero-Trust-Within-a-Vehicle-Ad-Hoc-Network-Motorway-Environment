#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/node.h"
#include <iostream>
#include <string>

namespace ns3{
    class TargetHeader : public Header{
        public:

            TargetHeader();
            TargetHeader(uint16_t data, uint32_t nodeId, uint32_t serialNo);
            virtual ~TargetHeader();

            static TypeId GetTypeId(void);
            virtual TypeId GetInstanceTypeId(void) const;
            virtual uint32_t GetSerializedSize(void) const;
            virtual void Serialize(Buffer::Iterator start) const;
            virtual uint32_t Deserialize(Buffer::Iterator start);
            virtual void Print(std::ostream &os) const;

            void setMessageType(uint16_t messageType);
            void setNodeId(uint32_t nodeId);
            void setSerial(uint32_t serial);
            uint16_t getMessageType();
            uint32_t getNodeId();
            uint32_t getSerial();

        private:
            uint16_t m_messageType;
            uint16_t m_nodeId;
            uint32_t m_serial;

    };
}//namespace ns3
