#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/node.h"
#include <iostream>
#include <string>

namespace ns3{
    class WaveReplyHeader : public Header{
        public:

            WaveReplyHeader();
            WaveReplyHeader(uint32_t targetSerial,uint32_t sequenceNumber, uint16_t expectReply);
            virtual ~WaveReplyHeader();

            static TypeId GetTypeId(void);
            virtual TypeId GetInstanceTypeId(void) const;
            virtual uint32_t GetSerializedSize(void) const;
            virtual void Serialize(Buffer::Iterator start) const;
            virtual uint32_t Deserialize(Buffer::Iterator start);
            virtual void Print(std::ostream &os) const;

            void setTarget(uint32_t data);
            void setSeq(uint32_t);
            void setExpectReply(uint16_t yesNo);

            uint32_t getTarget();
            uint32_t getSeq();
            uint16_t getExpectReply();

        private:
            uint32_t m_target;
            uint32_t m_seq;
            uint16_t m_expectReply;

    };
}//namespace ns3
