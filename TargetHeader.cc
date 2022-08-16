#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include <iostream>

#include "TargetHeader.h"
#include "ns3/node.h"

namespace ns3{

    TargetHeader::TargetHeader(){}

    /*
        message type is one of the following: {18, 21, 27, 42, 43}
                21 - original advertisement from RSU
                18 - reply from VehicleApp requesting to auth
                27 - reply from RSU in resposne to auth request
                42 - auth inquiry coming from WaveApp via VehicleApp
                43 - response to auth inquiry

        nodeId used to attach to that node's socket
        data is the RSU's serial number
    */
    TargetHeader::TargetHeader(uint16_t messageType, uint32_t nodeId, uint32_t serialNo){
        m_messageType = messageType;
        m_nodeId = nodeId;
        m_serial = serialNo;
    }

    TargetHeader::~TargetHeader (){}

    TypeId TargetHeader::GetTypeId (void){
          static TypeId tid = TypeId ("ns3::TargetHeader")
            .SetParent<Header> ()
            .AddConstructor<TargetHeader> ()
            ;
          return tid;
    }

    TypeId TargetHeader::GetInstanceTypeId (void) const{
          return TargetHeader::GetTypeId ();
    }

    void TargetHeader::Print (std::ostream &os) const{
          os << "data=" << m_messageType;
          os << "; nodeId=" << m_nodeId;
          os << "; serialNo=" << m_serial;
    }

    uint32_t TargetHeader::GetSerializedSize (void) const{
          return sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
    }

    void TargetHeader::Serialize (Buffer::Iterator start) const{
          start.WriteU16 (m_messageType);
          start.WriteU32 (m_nodeId);
          start.WriteU32 (m_serial);
    }

    uint32_t TargetHeader::Deserialize (Buffer::Iterator start){
          m_messageType = start.ReadU16 ();
          m_nodeId = start.ReadU32 ();
          m_serial = start.ReadU32 ();

          return GetSerializedSize();
    }

    void TargetHeader::setMessageType (uint16_t messageType){
        m_messageType = messageType;
    }

    void TargetHeader::setNodeId (uint32_t nodeId){
        m_nodeId = nodeId;
    }

    void TargetHeader::setSerial (uint32_t serialNo){
        m_serial = serialNo;
    }

    uint16_t TargetHeader::getMessageType (void){
        return m_messageType;
    }

    uint32_t TargetHeader::getNodeId(){
        return m_nodeId;
    }

    uint32_t TargetHeader::getSerial(){
        return m_serial;
    }
}//namespace ns3
