/*==============================================================================
  packet_manager.h
  ------------------------------------------------------------------------------
  Define estructuras DataPacket, AckPacket, HelloPacket y AltPacket.
  – Genera nodeID y messageID únicos.
  – Serializa / deserializa paquetes según el primer byte (messageType).
==============================================================================*/
#ifndef PACKET_MANAGER_H
#define PACKET_MANAGER_H

#include "config.h"
#include <stdint.h>
#include <string.h>  //memcpy()
#include <esp_system.h> //ESP.getEfuseMac()

/*----------------------------------------------------------------------------*/
/*  Estructuras de paquete                                                    */
/*----------------------------------------------------------------------------*/
struct DataPacket {
    uint8_t messageType;     
    uint16_t meshID;         
    uint32_t messageID;      
    uint16_t originNode;     
    uint16_t destinationNode;
    uint16_t nextHop;        
    uint8_t extra;        
    uint8_t ttl;             
    uint32_t payload;        
};
struct AckPacket {
    uint8_t messageType;     
    uint16_t meshID;         
    uint32_t messageID;      
    uint16_t originNode;     
    uint16_t destinationNode;
};
struct HelloPacket {
    uint8_t  messageType;  
    uint16_t meshID;       
    uint32_t messageID;    
    uint16_t originNode;   
};
struct AltPacket {
    uint8_t messageType;
    uint16_t meshID;
    uint32_t messageID;
    uint16_t originNode;
    uint16_t destinationNode;
};
/*----------------------------------------------------------------------------*/
/*  Pendiente de ACK                                                          */
/*----------------------------------------------------------------------------*/
struct PendingAck {
    DataPacket packet;
    unsigned long timestamp;
    uint8_t retryCount;      
};

/*============================================================================*/
/*  Identificación de nodo y mensaje                                          */
/*============================================================================*/
inline uint16_t getNodeID() {
    uint64_t chipId = ESP.getEfuseMac();
    uint16_t nodeId = (uint16_t)((chipId & 0xFFFF) ^ ((chipId >> 32) & 0xFFFF)); 
    return nodeId;
}
inline uint32_t getMessageID(uint8_t messageType) {
    // Genera un número aleatorio de 8 bits
    uint8_t randomNumber = random(0, 256);
    uint16_t nodeId = getNodeID();
    uint32_t messageID = ((uint32_t)messageType << 24) | ((uint32_t)nodeId << 8) | ((uint32_t)(randomNumber & 0xFF));
    return messageID;
}
/*============================================================================*/
/*  Helpers de rellenado                                                      */
/*============================================================================*/
inline void fillDataPacket(DataPacket &packet, uint8_t messageType, uint16_t meshID, uint32_t messageID,
                    uint16_t originNode, uint16_t destinationNode, uint16_t nextHop, 
                    uint8_t extra, uint8_t ttl, uint32_t payload) {
    packet.messageType = messageType;
    packet.meshID = meshID;
    packet.messageID = messageID;
    packet.originNode = originNode;
    packet.destinationNode = destinationNode;
    packet.nextHop = nextHop;
    packet.extra = extra;
    packet.ttl = ttl;
    packet.payload = payload;
}
inline void fillDataPacket(DataPacket &packet, uint16_t destinationNode, uint16_t nextHop, 
                    uint8_t extra, uint8_t ttl, uint32_t payload) {
    packet.messageType = MESSAGE_TYPE_DATA;
    packet.meshID = MESH_ID;
    packet.messageID = getMessageID(MESSAGE_TYPE_DATA);
    packet.originNode = getNodeID();
    packet.destinationNode = destinationNode;
    packet.nextHop = nextHop;
    packet.extra = extra;
    packet.ttl = ttl;
    packet.payload = payload;
}
inline void fillAckPacket(AckPacket &packet, uint32_t messageID, uint16_t destinationNode) {
    packet.messageType = MESSAGE_TYPE_ACK;
    packet.meshID = MESH_ID;
    packet.messageID = messageID;
    packet.originNode = getNodeID();
    packet.destinationNode = destinationNode;
}
inline void fillHelloPacket(HelloPacket &pkt) {
    pkt.messageType = MESSAGE_TYPE_HELLO;
    pkt.meshID      = MESH_ID;
    pkt.messageID   = getMessageID(MESSAGE_TYPE_HELLO);
    pkt.originNode  = getNodeID();
}
inline void fillAltPacket(AltPacket &packet,uint32_t messageID,uint16_t destinationNode) {
    packet.messageType = MESSAGE_TYPE_ALT;
    packet.meshID = MESH_ID;
    packet.messageID = messageID;
    packet.originNode = getNodeID();
    packet.destinationNode = destinationNode;
}
/*============================================================================*/
/*  Serialización / deserialización                                           */
/*============================================================================*/
inline void serializePacket(const void *packet, uint8_t *buffer) {
    if (packet == nullptr || buffer == nullptr) {
        return;
    }
    uint8_t messageType = *(reinterpret_cast<const uint8_t *>(packet));

    switch (messageType) {
        case MESSAGE_TYPE_DATA:
            memcpy(buffer, packet, sizeof(DataPacket));
            break;
        case MESSAGE_TYPE_ACK:
            memcpy(buffer, packet, sizeof(AckPacket));
            break;
        case MESSAGE_TYPE_HELLO:
            memcpy(buffer, packet, sizeof(HelloPacket));
            break;
        case MESSAGE_TYPE_ALT:
            memcpy(buffer, packet, sizeof(AltPacket));
            break;
        default:
            break;
    }
}

inline void deserializePacket(void *packet, const uint8_t *buffer) {
    if (packet == nullptr || buffer == nullptr) {
        return;
    }
    uint8_t messageType = buffer[0];
    switch (messageType) {
        case MESSAGE_TYPE_DATA:
            memcpy(packet, buffer, sizeof(DataPacket));
            break;
        case MESSAGE_TYPE_ACK:
            memcpy(packet, buffer, sizeof(AckPacket));
            break;
        case MESSAGE_TYPE_HELLO:
            memcpy(packet, buffer, sizeof(HelloPacket));
            break;
        case MESSAGE_TYPE_ALT:
            memcpy(packet, buffer, sizeof(AltPacket));
            break;
        default:
            break;
    }
}

#endif
