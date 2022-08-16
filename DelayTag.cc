#include "DelayTag.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
	NS_LOG_COMPONENT_DEFINE("DelayTag");
	NS_OBJECT_ENSURE_REGISTERED (DelayTag);

	DelayTag::DelayTag() {}

	DelayTag::DelayTag(uint32_t senderNodeId, int64_t timeCreated) {
		m_senderNode = senderNodeId;
		m_timestamp = timeCreated;
	}

	DelayTag::~DelayTag() {
	}

	//Almost all custom tags will have similar implementation of GetTypeId and GetInstanceTypeId
	TypeId DelayTag::GetTypeId (void){
	  static TypeId tid = TypeId ("ns3::DelayTag")
	    .SetParent<Tag> ()
	    .AddConstructor<DelayTag> ();
	  return tid;
	}

	TypeId DelayTag::GetInstanceTypeId (void) const{
	  return DelayTag::GetTypeId ();
	}

	uint32_t DelayTag::GetSerializedSize (void) const{
		return sizeof(uint32_t) + sizeof(uint64_t);
	}


	//order should match deserialise
	void DelayTag::Serialize (TagBuffer i) const{
		i.WriteU32(m_senderNode);
		i.WriteU64(m_timestamp);

	}

	//match serialise
	void DelayTag::Deserialize (TagBuffer i){
		m_senderNode = i.ReadU32();
		m_timestamp = i.ReadU32();
	}

	void DelayTag::Print (std::ostream &os) const{
	  os << " Node#: " << m_senderNode;
	  os << " timestamp: " << m_timestamp;
	}

	//setters / getters
	uint64_t DelayTag::getTime() {
		return m_timestamp;
	}

	uint32_t DelayTag::getSender() {
		return m_senderNode;
	}

	void DelayTag::setSender(uint32_t sender) {
		m_senderNode = sender;
	}
}//namespace ns3
