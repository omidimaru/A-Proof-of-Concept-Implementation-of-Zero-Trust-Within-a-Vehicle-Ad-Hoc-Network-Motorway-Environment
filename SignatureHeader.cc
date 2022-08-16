#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include <iostream>

#include "SignatureHeader.h"
#include "ns3/node.h"

namespace ns3{
    SignatureHeader::SignatureHeader(){}

    SignatureHeader::SignatureHeader(uint32_t rsuSerial, uint32_t vehicleSerial){
        m_rsuSerial = rsuSerial;
        m_vehicleSerial = vehicleSerial;
        m_registered = 0;
        m_timestamp = Simulator::Now().GetTimeStep();
    }

    SignatureHeader::SignatureHeader(uint32_t rsuSerial, uint32_t vehicleSerial, uint16_t registered){
        m_rsuSerial = rsuSerial;
        m_vehicleSerial = vehicleSerial;
        m_registered = registered;
        m_timestamp = Simulator::Now().GetTimeStep();
    }

    SignatureHeader::~SignatureHeader (){}

    TypeId SignatureHeader::GetTypeId (void){
          static TypeId tid = TypeId ("ns3::SignatureHeader")
            .SetParent<Header> ()
            .AddConstructor<SignatureHeader> ()
            ;
          return tid;
    }

    TypeId SignatureHeader::GetInstanceTypeId (void) const{
          return SignatureHeader::GetTypeId ();
    }

    void SignatureHeader::Print (std::ostream &os) const{
          os << "registered=" << m_registered;
          os << "; rsuSerial=" << m_rsuSerial;
          os << "; vehicleSerial=" << m_vehicleSerial;
          os << "; timestamp=" << m_timestamp;
    }

    uint32_t SignatureHeader::GetSerializedSize (void) const{
          return sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t);
    }

    void SignatureHeader::Serialize (Buffer::Iterator i) const{
          i.WriteU16(m_registered);
          i.WriteU32 (m_rsuSerial);
          i.WriteU32 (m_vehicleSerial);
          i.WriteU64(m_timestamp);
    }

    uint32_t SignatureHeader::Deserialize (Buffer::Iterator i){
        m_registered = i.ReadU16();
        m_rsuSerial = i.ReadU32();
        m_vehicleSerial = i.ReadU32();
        m_timestamp =  i.ReadU64();

        return GetSerializedSize();
    }

    uint32_t SignatureHeader::getRsuSerial() {
    	return m_rsuSerial;
    }

    void SignatureHeader::setRsuSerial(uint32_t serial) {
    	m_rsuSerial = serial;
    }

    uint32_t SignatureHeader::getVehicleSerial() {
    	return m_vehicleSerial;
    }

    void SignatureHeader::setVehicleSerial(uint32_t serial) {
    	m_vehicleSerial = serial;
    }

    uint64_t SignatureHeader::getTimestamp() {
    	return m_timestamp;
    }

    void SignatureHeader::setTimestamp(uint64_t t) {

    	m_timestamp = t;
    }

    uint16_t SignatureHeader::getRegistered(){
        return m_registered;
    }

    void SignatureHeader::setRegistered(uint16_t yesNo){
        m_registered = yesNo;
    }


}//namespace ns3
