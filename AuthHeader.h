#ifndef AUTH_HEADER_H
#define AUTH_HEADER_H

#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/node.h"
#include <iostream>
#include <string>

namespace ns3{
    class AuthHeader : public Header{
        public:

            AuthHeader();
            AuthHeader(uint16_t messageType, uint16_t authCode, uint32_t reqSerial, uint32_t nodeId);
            virtual ~AuthHeader();

            static TypeId GetTypeId(void);
            virtual TypeId GetInstanceTypeId(void) const;
            virtual uint32_t GetSerializedSize(void) const;
            virtual void Serialize(Buffer::Iterator start) const;
            virtual uint32_t Deserialize(Buffer::Iterator start);
            virtual void Print(std::ostream &os) const;

            void setMessageType(uint16_t messageType);
            void setAuthCode(uint16_t authCode);
            void setNodeId(uint32_t nodeId);
            void setRequestedSerial(uint32_t reqSerial);

            uint16_t getMessageType();
            uint16_t getAuthCode();
            uint32_t getNodeId();
            uint32_t getRequestedSerial();

        private:
            uint16_t m_messageType;
            uint16_t m_authCode;
            uint32_t m_requestedSerial;
            uint32_t m_nodeId;

    };
}//namespace ns3
#endif
