#include "WaveReplyHeader.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
	NS_LOG_COMPONENT_DEFINE("WaveReplyHeader");
	NS_OBJECT_ENSURE_REGISTERED (WaveReplyHeader);

	WaveReplyHeader::WaveReplyHeader() {
		m_target = -1;
	}

	WaveReplyHeader::WaveReplyHeader(uint32_t targetSerial, uint32_t sequenceNum, uint16_t expectReply) {
		m_target = targetSerial;
		m_seq = sequenceNum;
		m_expectReply = expectReply;
	}

	WaveReplyHeader::~WaveReplyHeader() {
	}

	TypeId WaveReplyHeader::GetTypeId (void){
	  static TypeId tid = TypeId ("ns3::WaveReplyHeader")
	    .SetParent<Header> ()
	    .AddConstructor<WaveReplyHeader> ();
	  return tid;
	}

    TypeId WaveReplyHeader::GetInstanceTypeId (void) const{
          return WaveReplyHeader::GetTypeId ();
    }

    void WaveReplyHeader::Print (std::ostream &os) const{
          os << "targetSerial=" << m_target;
		  os << "; sequenceNumber=" << m_seq;
		  os << "; expectReply=" << m_expectReply;
    }

    uint32_t WaveReplyHeader::GetSerializedSize (void) const{
          return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint16_t);
    }

    void WaveReplyHeader::Serialize (Buffer::Iterator i) const{
          i.WriteU32(m_target);
		  i.WriteU32(m_seq);
		  i.WriteU16(m_expectReply);
    }

    uint32_t WaveReplyHeader::Deserialize (Buffer::Iterator i){
        m_target = i.ReadU32();
		m_seq = i.ReadU32();
		m_expectReply = i.ReadU16();

        return GetSerializedSize();
    }

	//setters / getters
	uint32_t WaveReplyHeader::getTarget() {
		return m_target;
	}

	void WaveReplyHeader::setTarget(uint32_t target) {
		m_target = target;
	}

	uint32_t WaveReplyHeader::getSeq() {
		return m_seq;
	}

	void WaveReplyHeader::setSeq(uint32_t seq) {
		m_seq = seq;
	}

	uint16_t WaveReplyHeader::getExpectReply() {
		return m_expectReply;
	}

	void WaveReplyHeader::setExpectReply(uint16_t yesNo) {
		m_expectReply = yesNo;
	}
}//namespace ns3
