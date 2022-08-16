#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/node.h"

#include "AuthHeader.h"

#include <iostream>

namespace ns3{

    AuthHeader::AuthHeader(){}

    /*
        message type is one of the following: {42, 43}
                42 - auth inquiry coming from WaveApp via VehicleApp
                43 - response to auth inquiry

        nodeId used to attach to that node's socket
        data can either be the serial of the RSU for the original authentication
            or a code for whether the requested vehicle has authenticated with the RSU recently
                    11 - registered and recognised
                    10 - registered and unrecognised
                    0  - not part of RSU's network
    */
    AuthHeader::AuthHeader(uint16_t messageType, uint16_t authCode, uint32_t reqSerial, uint32_t nodeId){
        m_messageType = messageType;
        m_authCode = authCode;
        m_requestedSerial = reqSerial;
        m_nodeId = nodeId;
    }

    AuthHeader::~AuthHeader (){}

    TypeId AuthHeader::GetTypeId (void){
          static TypeId tid = TypeId ("ns3::AuthHeader")
            .SetParent<Header> ()
            .AddConstructor<AuthHeader> ()
            ;
          return tid;
    }

    TypeId AuthHeader::GetInstanceTypeId (void) const{
          return AuthHeader::GetTypeId ();
    }

    void AuthHeader::Print (std::ostream &os) const{
          os << "messageType=" << m_messageType;
          os << "; authCode=" << m_authCode;
          os << "; requestedSerial=" << m_requestedSerial;
          os << "; nodeId=" << m_nodeId;
    }

    uint32_t AuthHeader::GetSerializedSize (void) const{
          return sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t);
    }

    void AuthHeader::Serialize (Buffer::Iterator start) const{
          start.WriteU16 (m_messageType);
          start.WriteU16 (m_authCode);
          start.WriteU32 (m_requestedSerial);
          start.WriteU32 (m_nodeId);
    }

    uint32_t AuthHeader::Deserialize (Buffer::Iterator start){
          m_messageType = start.ReadU16 ();
          m_authCode = start.ReadU16 ();
          m_requestedSerial = start.ReadU32 ();
          m_nodeId = start.ReadU32 ();

          return GetSerializedSize();
    }



    void AuthHeader::setMessageType (uint16_t messageType){
        m_messageType = messageType;
    }

    void AuthHeader::setAuthCode (uint16_t authCode){
        m_authCode = authCode;
    }

    void AuthHeader::setNodeId (uint32_t nodeId){
        m_nodeId = nodeId;
    }

    void AuthHeader::setRequestedSerial (uint32_t reqSerial){
        m_requestedSerial = reqSerial;
    }

    uint16_t AuthHeader::getMessageType (){
        return m_messageType;
    }

    uint16_t AuthHeader::getAuthCode(){
        return m_authCode;
    }

    uint32_t AuthHeader::getNodeId(){
        return m_nodeId;
    }

    uint32_t AuthHeader::getRequestedSerial(){
        return m_requestedSerial;
    }
}//namespace ns3
