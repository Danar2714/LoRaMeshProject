/*==============================================================================
  communication_manager.h
  ------------------------------------------------------------------------------
  Capa de enlace entre la lógica de alto nivel y el driver LoRa.
  – Registra los eventos de TX/RX con SX1262.
  – Proporciona funciones para enviar paquetes y filtrar recepción.
  – Contiene utilidades de depuración (impresiones Serial).
==============================================================================*/
#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include "config.h"
#include "lora_manager.h"
#include "packet_manager.h"
#include "routing_manager.h"
#include "Arduino.h"
#include <string.h>  // memcpy()

/*----------------------------------------------------------------------------*/
/*  Declaraciones adelantadas (evitan dependencia circular)                   */
/*----------------------------------------------------------------------------*/
void scheduleAckMessage(uint32_t messageID, uint16_t destinationNode); // message_scheduler.h
void scheduleMessage(uint32_t payload); // message_scheduler.h
void scheduleAltMessage(uint32_t messageID, uint16_t destinationNode); // message_scheduler.h
bool checkDuplicates(uint32_t messageID); // message_receiver.h
bool isPendingAck(uint32_t messageID); // message_receiver.h
void addMessageIDAfterAck(uint32_t messageID); // message_receiver.h
void reEnqueueAlternateRoute(const DataPacket &originalPacket, uint16_t excludeNeighbor, bool removeNeighborFlag);
bool recentlyAcked(uint32_t messageID);   // message_scheduler.h


/*----------------------------------------------------------------------------*/
/*  Variables globales compartidas                                            */
/*----------------------------------------------------------------------------*/
extern volatile bool loraIdle;
extern volatile bool transmissionDone;   // Bandera para saber si la transmisión se completó
extern volatile bool receptionDone;      // Bandera para saber si la recepción se completó
extern volatile bool transmissionError;  // Bandera para saber si hubo un error en la transmisión

extern DataPacket receivedPacket;
DataPacket scheduledDataPacket;

extern uint8_t receivedBuffer[MAX_PACKET_SIZE];
extern uint16_t receivedSize;
extern int16_t receivedRssi;
extern int8_t receivedSnr;

extern LoraManager loraAntena;
extern PendingAck pendingAcks[MAX_PENDING_ACKS];

/*============================================================================*/
/*  Callbacks de radio (registrados en RadioEvents_t)                          */
/*============================================================================*/
inline void OnTxDone() {
    transmissionDone = true; 
    loraIdle = true;         
}

inline void OnTxTimeout() {
    transmissionError = true; 
    loraIdle = true;          
}

inline void OnRxDone(uint8_t *rxBuffer, uint16_t size, int16_t rssi, int8_t snr) {
    /* Se descarta si el buffer excede el máximo permitido */
    if (size > MAX_PACKET_SIZE) {
        receptionDone = false;
        loraIdle = true;
        return;
    }
    /* Copia segura a los buffers globales */
    memcpy(receivedBuffer, rxBuffer, size);
    receivedSize = size;
    receivedRssi = rssi;
    receivedSnr = snr;

    receptionDone = true;
    loraIdle = true; 
}

/*----------------------------------------------------------------------------*/
/*  Registro de callbacks de Radio (driver)                                      */
/*----------------------------------------------------------------------------*/
inline void initTxRxEvents(RadioEvents_t &events) {
    events.TxDone = OnTxDone;
    events.RxDone = OnRxDone;
    events.TxTimeout = OnTxTimeout;
}

/*============================================================================*/
/*  Funciones de transmisión (sobrecarga según tipo)                           */
/*============================================================================*/
inline void handleTransmission(const DataPacket &packet) {
    uint8_t txBuffer[sizeof(DataPacket)];
    serializePacket(&packet, txBuffer);  
    loraAntena.send(txBuffer, sizeof(DataPacket));  
    loraIdle = false;                   
}

inline void handleTransmission(const AckPacket &packet) {
    uint8_t txBuffer[sizeof(AckPacket)];
    serializePacket(&packet, txBuffer);  
    loraAntena.send(txBuffer, sizeof(AckPacket));  
    loraIdle = false;                   
}

inline void handleTransmission(const HelloPacket &packet) {
    uint8_t txBuffer[sizeof(HelloPacket)];
    serializePacket(&packet, txBuffer);  
    loraAntena.send(txBuffer, sizeof(HelloPacket));
    loraIdle = false;  
}
inline void handleTransmission(const AltPacket &packet) {
    uint8_t txBuffer[sizeof(AltPacket)];
    serializePacket(&packet, txBuffer);
    loraAntena.send(txBuffer, sizeof(AltPacket));
    loraIdle = false;
}

/*----------------------------------------------------------------------------*/
/*  Recepción pasiva                                                          */
/*----------------------------------------------------------------------------*/
inline void handleReception() {
    loraAntena.receive();  
}
/*----------------------------------------------------------------------------*/
/*  Utilidad de depurado (impresión detallada)                                */
/*----------------------------------------------------------------------------*/
inline void printReceivedPacket() {
    Serial.println("Paquete recibido:");
    Serial.printf("Tipo de mensaje: %d\n", receivedPacket.messageType);
    Serial.printf("ID de malla: %d\n", receivedPacket.meshID);
    Serial.printf("ID del mensaje: %u\n", receivedPacket.messageID);
    Serial.printf("Nodo origen: %d\n", receivedPacket.originNode);
    Serial.printf("Nodo destino: %d\n", receivedPacket.destinationNode);
    Serial.printf("Siguiente salto: %d\n", receivedPacket.nextHop);
    Serial.printf("Extra: %d\n", receivedPacket.extra);
    Serial.printf("TTL: %d\n", receivedPacket.ttl);
    Serial.printf("Payload (contador): %u\n", receivedPacket.payload);
    Serial.printf("RSSI: %d\n", receivedRssi);
}

/*============================================================================*/
/*  Filtros: decide si un paquete debe descartarse en este nodo                */
/*============================================================================*/
inline bool dropPacket(const DataPacket &packet, uint16_t localMeshID, uint16_t localNodeID) {
    if (packet.ttl==0){
      return true; 
    }
    if (packet.meshID != localMeshID) {
        return true;
    }
    if (packet.nextHop == localNodeID || (packet.destinationNode == localNodeID && packet.nextHop == localNodeID)) {
        return false;
    }
    if (packet.destinationNode == localNodeID && packet.nextHop != localNodeID) {
        return true; 
    }
    if (packet.destinationNode != localNodeID && packet.nextHop != localNodeID) {
        return true; 
    }
    return false;
}

inline bool dropAckPacket(const AckPacket &packet, uint16_t localMeshID, uint16_t localNodeID) {
    if (packet.meshID != localMeshID) {
        return true; 
    }
    if (packet.destinationNode != localNodeID) {
        return true;
    }
    return false;
}

inline bool dropHelloPacket(const HelloPacket &packet, uint16_t localMeshID) {
    if (packet.meshID != localMeshID){
      return true;
    } 
    if (!isAllowedNeighbor(packet.originNode)) {
        return true;
    }
    return false;
}
inline bool dropAltPacket(const AltPacket &packet, uint16_t localMeshID, uint16_t localNodeID) {
    if (packet.meshID != localMeshID) {
        return true;
    }
    if (packet.destinationNode != localNodeID) {
        return true;
    }
    return false;
}
/*============================================================================*/
/*  Procesamiento de receivedBuffer según tipo de mensaje                      */
/*============================================================================*/
inline void processPayload() {
  uint8_t messageType = receivedBuffer[0];

  switch (messageType) {
    /*====================================================================
          DATA (1) – manejo completo: duplicados, ACK, TTL, reenvío
    ====================================================================*/
    case MESSAGE_TYPE_DATA:
      {
        deserializePacket(&receivedPacket, receivedBuffer);
        if (dropPacket(receivedPacket, MESH_ID, getNodeID())) {
          return;
        }
        /*-- Duplicados --------------------------------------------------*/
        bool isDup = checkDuplicates(receivedPacket.messageID);
        if (isDup == true) {
            if (recentlyAcked(receivedPacket.messageID)) {      
                Serial.println("DATA duplicado; ACK ya enviado → replay ACK");
                scheduleAckMessage(receivedPacket.messageID,receivedPacket.originNode);
                return;
            }
            if (isPendingAck(receivedPacket.messageID)) {
                Serial.println("DATA duplicado propio => ignorado (ACK pendiente).");
                return;
            }
            Serial.printf("DATA duplicado ID=%u => Enviar ALT.\n", receivedPacket.messageID);
            scheduleAltMessage(receivedPacket.messageID, receivedPacket.originNode);
            return;
        }
        /*-- Procesamiento normal ---------------------------------------*/
        Serial.println("Procesando el paquete de datos recibido...");
        printReceivedPacket();
        /* Programar ACK hop-by-hop */
        scheduleAckMessage(receivedPacket.messageID, receivedPacket.originNode);
        /* Reenvío si no soy destino final */
        receivedPacket.ttl--;
        if (receivedPacket.destinationNode == getNodeID()) {
          Serial.println("Soy el destino final. No reenvío.");
          return;
        }
        if (receivedPacket.ttl > 0) {
          uint16_t previousHop = receivedPacket.originNode;
          receivedPacket.originNode = getNodeID();
          receivedPacket.nextHop = getNextHop(getNodeID(),receivedPacket.destinationNode,previousHop);
          Serial.printf("Reenviar => new nextHop=%u ttl=%d\n", receivedPacket.nextHop, receivedPacket.ttl);
          scheduledDataPacket = receivedPacket;
          scheduleMessage(receivedPacket.payload);
        } else {
          Serial.println("TTL=0. No se reenvía.");
        }
        break;
      }
    /*====================================================================
          ACK (2)
    ====================================================================*/
    case MESSAGE_TYPE_ACK:
      {
        AckPacket ackPacket;
        deserializePacket(&ackPacket, receivedBuffer);
        if (dropAckPacket(ackPacket, MESH_ID, getNodeID())) {
          return;
        }
        Serial.printf("ACK recibido para messageID: %u\n", ackPacket.messageID);
        Serial.printf("  → Origen del ACK: %u\n", ackPacket.originNode);
        Serial.printf("  → Destino del ACK: %u\n", ackPacket.destinationNode);
        Serial.printf("  → Nodo actual: %u\n", getNodeID());

        /* Marca como atendido en pendingAcks */
        for (int i = 0; i < MAX_PENDING_ACKS; i++) {
          if (pendingAcks[i].packet.messageID == ackPacket.messageID) {
            pendingAcks[i].timestamp = 0;
            Serial.printf("ACK procesado y eliminado de la lista de pendientes: %u\n",ackPacket.messageID);
            break;
          }
        }
        addMessageIDAfterAck(ackPacket.messageID);
        break;
      }
    /*====================================================================
          HELLO (3) – actualiza tabla de vecinos
    ====================================================================*/
    case MESSAGE_TYPE_HELLO:
      {
        HelloPacket helloPacket;
        deserializePacket(&helloPacket, receivedBuffer);
        if (dropHelloPacket(helloPacket, MESH_ID)) {
          return;
        }
        Serial.println("HELLO recibido!");
        Serial.printf("  originNode: %u  RSSI: %d\n", helloPacket.originNode, receivedRssi);
        addOrUpdateNeighbor(helloPacket.originNode, receivedRssi);
        break;
      }
    /*====================================================================
          ALT (4) – petición de ruta alterna para DATA duplicado
    ====================================================================*/
    case MESSAGE_TYPE_ALT:
      {
        AltPacket altPacket;
        deserializePacket(&altPacket, receivedBuffer);
        if (dropAltPacket(altPacket, MESH_ID, getNodeID())) {
            return;
        }
        Serial.printf("ALT recibido => msgID=%u, originALT=%u, meEnvio=%u\n",altPacket.messageID,altPacket.originNode,altPacket.destinationNode);
        /* Se reubica el DATA original para nuevo intento */
        for (int i = 0; i < MAX_PENDING_ACKS; i++) {
            if (pendingAcks[i].timestamp != 0 && pendingAcks[i].packet.messageID == altPacket.messageID) {
                DataPacket original = pendingAcks[i].packet;
                pendingAcks[i].timestamp = 0;
                pendingAcks[i].retryCount = 0;
                reEnqueueAlternateRoute(original, altPacket.originNode, false);
                break;
            }
        }
        break;
      }
    /* tipo desconocido */
    default:
      break;
  }
}
#endif


