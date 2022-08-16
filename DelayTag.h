#ifndef DELAY_TAG_H
#define DELAY_TAG_H

#include "ns3/tag.h"
#include "ns3/vector.h"
#include "ns3/nstime.h"

#include <string>

namespace ns3
{
	class DelayTag : public Tag {
	public:

		DelayTag();
		DelayTag(uint32_t senderNodeId, int64_t timeCreated);
		virtual ~DelayTag();

		static TypeId GetTypeId(void);
		virtual TypeId GetInstanceTypeId(void) const;
		virtual uint32_t GetSerializedSize(void) const;
		virtual void Serialize (TagBuffer i) const;
		virtual void Deserialize (TagBuffer i);
		virtual void Print (std::ostream & os) const;

		uint32_t getSender();
		uint64_t getTime();
		void setSender(uint32_t sender);

	private:
		uint32_t m_senderNode;
		uint64_t m_timestamp;
	};
}//namespace ns3
#endif
