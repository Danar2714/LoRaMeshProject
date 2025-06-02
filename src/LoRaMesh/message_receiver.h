/*==============================================================================
  message_receiver.h
  ------------------------------------------------------------------------------
  Lógica de recepción y filtrado de mensajes.
  – Verifica duplicados, ACK pendientes y ventana de colisión.
  – Expone processReceivedMessage(), addMessageID(), isPendingAck(), etc.
==============================================================================*/
#ifndef MESSAGE_RECEIVER_H
#define MESSAGE_RECEIVER_H

#include "config.h"
#include "packet_manager.h"
#include "communication_manager.h"

/*----------------------------------------------------------------------------*/
/*  Declaración adelantada (planificador)                                     */
/*----------------------------------------------------------------------------*/
void increaseWaitTime(); //message_schedualer.h

/*----------------------------------------------------------------------------*/
/*  Historial de messageID (detección de duplicados)                          */
/*----------------------------------------------------------------------------*/
static uint32_t messageIDHistory[MAX_DUPLICATE_HISTORY];
static int idHistoryIndex = 0;

/*----------------------------------------------------------------------------*/
/*  Gestión del historial                                                     */
/*----------------------------------------------------------------------------*/
inline void addMessageID(uint32_t messageID) {
    messageIDHistory[idHistoryIndex] = messageID;
    idHistoryIndex++;
    if(idHistoryIndex >= MAX_DUPLICATE_HISTORY) {
        idHistoryIndex = 0;
    }
}

inline bool checkDuplicates(uint32_t messageID) {
    for(int i=0; i<MAX_DUPLICATE_HISTORY; i++) {
        if(messageIDHistory[i] == messageID) {
            return true; 
        }
    }
    return false;
}

inline void addMessageIDAfterAck(uint32_t messageID) {
    for(int i = 0; i < MAX_DUPLICATE_HISTORY; i++){
        if(messageIDHistory[i] == messageID){
            return;
        }
    }
    addMessageID(messageID);
    Serial.printf("[addMessageIDAfterAck] Se añade messageID=%u al historial de duplicados.\n", messageID);
}

/*----------------------------------------------------------------------------*/
/*  Consulta de ACK pendientes                                                */
/*----------------------------------------------------------------------------*/
inline bool isPendingAck(uint32_t messageID) {
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (pendingAcks[i].timestamp != 0 &&
            pendingAcks[i].packet.messageID == messageID) {
            return true;    
        }
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/*  Inicialización (vacía, pero se deja por simetría)                         */
/*----------------------------------------------------------------------------*/
inline void initMessageReceiver() {
}

/*============================================================================*/
/*  processReceivedMessage(): procesa si receptionDone == true                */
/*============================================================================*/
inline uint8_t processReceivedMessage(unsigned long &oledDisplayTime) {
    if (receptionDone) {
        uint8_t receivedType = receivedBuffer[0]; 
        processPayload();
        receptionDone = false;
        increaseWaitTime(); // aleatoriza back-off
        oledDisplayTime = millis();
        return receivedType;
    }
    return 0;
}

/*============================================================================*/
/*  Ventana de escucha (LBT)                                                  */
/*============================================================================*/
inline void windowCollisionPrevention() {
  for (int attempt = 1; attempt <= MAX_WINDOW_RETRIES; attempt++) {
    receptionDone = false;
    Serial.printf("windowCollisionPrevention => Escuchando %u ms...\n", (unsigned)LISTEN_WINDOW_MS);
    unsigned long startTime = millis();
    bool gotPacketInWindow = false;
    while ((millis() - startTime) < LISTEN_WINDOW_MS) {
      delay(5);  
      if (receptionDone) {
        unsigned long packetInWindowTime = 0;
        processReceivedMessage(packetInWindowTime);
        gotPacketInWindow = true;
        break;
      }
    }
    if (gotPacketInWindow) {
      Serial.printf("Canal ocupado en la ventana %d => reintento...\n", attempt);
    } else {
      Serial.printf("No se recibió nada en la ventana %d => canal libre!\n", attempt);
      return;
    }
  }
  Serial.printf("Se alcanzó MAX_WINDOW_RETRIES=%d => enviamos de todas formas.\n",MAX_WINDOW_RETRIES);
}

#endif
