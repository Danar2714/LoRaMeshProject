/*==============================================================================
  message_scheduler.h
  ------------------------------------------------------------------------------
  Planificador y cola de mensajes:
  – Maneja DATA, ACK, HELLO y ALT con espera aleatoria.
  – Supervisa ACK pendientes y reintentos.
  – Implementa reenvío por ruta alterna y HELLO automático.
==============================================================================*/
#ifndef MESSAGE_SCHEDULER_H
#define MESSAGE_SCHEDULER_H

#include "config.h"
#include "packet_manager.h"
#include "communication_manager.h"
#include "routing_manager.h"

/*----------------------------------------------------------------------------*/
/*  Declaración adelantada                                                    */
/*----------------------------------------------------------------------------*/
void windowCollisionPrevention(); //Esta en message_receiver.h

/*============================================================================*/
/*  1) Historial para limitar re-enqueue por rutas alternas                    */
/*============================================================================*/
static uint32_t routeHistory[ROUTE_HISTORY_SIZE] = {0};
static uint8_t  routeCount  [ROUTE_HISTORY_SIZE] = {0};
static int      routeIdx = 0;

inline bool canReenqueue(uint32_t messageID)
{
    for (int i = 0; i < ROUTE_HISTORY_SIZE; i++) {
        if (routeHistory[i] == messageID) {               
            if (routeCount[i] >= ROUTE_MAX_ALTERNATES)    
                return false;
            routeCount[i]++;                              
            return true;
        }
    }
    routeHistory[routeIdx] = messageID;
    routeCount [routeIdx]  = 1;
    routeIdx++;
    if (routeIdx >= ROUTE_HISTORY_SIZE) routeIdx = 0;
    return true;
}
/*============================================================================*/
/*  2) Ventana para evitar reenvío múltiple de ACK (ACK replay)               */
/*============================================================================*/
struct AckReplayEntry {
    uint32_t messageID;
    unsigned long ts; // “timestamp” (marca de tiempo)
};
static AckReplayEntry ackReplay[ACK_REPLAY_WINDOW] = {0};
static int ackReplayPos = 0;

inline void rememberAckSent(uint32_t messageID) {
    ackReplay[ackReplayPos].messageID = messageID;
    ackReplay[ackReplayPos].ts    = millis();
    ackReplayPos++;
    if (ackReplayPos >= ACK_REPLAY_WINDOW) ackReplayPos = 0;
}
inline bool recentlyAcked(uint32_t messageID) {
    unsigned long now = millis();
    for (int i = 0; i < ACK_REPLAY_WINDOW; i++) {
        if (ackReplay[i].messageID == messageID &&
            (now - ackReplay[i].ts) <= ACK_REPLAY_TTL_MS) {
            return true;
        }
    }
    return false;
}

/*============================================================================*/
/*  3) Estructura de la cola de transmisión                                   */
/*============================================================================*/
struct ScheduledItem {
    bool isAck;               
    bool isHello;
    bool isAlt;              
    DataPacket data;           
    AckPacket ack;             
    HelloPacket hello;
    AltPacket alt; 
    unsigned long scheduleTime; 
    bool inUse;               
};

static ScheduledItem scheduledQueue[MAX_QUEUE_SIZE];
static int queueWriteIndex = 0; 

/*----------------------------------------------------------------------------*/
/*  Variables globales de apoyo                                               */
/*----------------------------------------------------------------------------*/
extern bool dataMessageSent;
PendingAck pendingAcks[MAX_PENDING_ACKS];
extern DataPacket scheduledDataPacket;

/*----------------------------------------------------------------------------*/
/*  HELLO periódico                                                           */
/*----------------------------------------------------------------------------*/
static unsigned long nextHelloTimeAuto = 0;

/*----------------------------------------------------------------------------*/
/*  Historial de ALT enviados (límite por messageID)                          */
/*----------------------------------------------------------------------------*/
static uint32_t altHistory[ALT_HISTORY_SIZE]   = {0};
static uint8_t  altCount  [ALT_HISTORY_SIZE]   = {0};
static int      altIdx = 0; //posicion actual


inline bool canSendAlt(uint32_t messageID) {
    for (int i = 0; i < ALT_HISTORY_SIZE; i++) {
        if (altHistory[i] == messageID) {            
            if (altCount[i] >= ALT_MAX_PER_MESSAGE) return false;
            altCount[i]++;                       
            return true;
        }
    }
    altHistory[altIdx] = messageID;
    altCount [altIdx]  = 1;
    altIdx++; 
    if (altIdx >= ALT_HISTORY_SIZE) {
        altIdx = 0;
    } 
    return true;
}


/*============================================================================*/
/*  4) Encolado de mensajes (DATA / ACK / HELLO / ALT)                        */
/*============================================================================*/
inline void enqueueDataMessage(uint32_t payload) {
    if (scheduledDataPacket.destinationNode == 0) {
        Serial.println("No se pudo encolar DATA: scheduledDataPacket.destinationNode = 0");
        return;
    }
    unsigned long randomWait = millis() + random(INITIAL_WAIT_LOWER, INITIAL_WAIT_UPPER);
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (!scheduledQueue[i].inUse) {
            scheduledQueue[i].isAck = false;
            scheduledQueue[i].isHello= false;
            scheduledQueue[i].isAlt  = false;
            
            scheduledQueue[i].data = scheduledDataPacket;
            

            scheduledQueue[i].scheduleTime = randomWait;
            scheduledQueue[i].inUse = true;

            scheduledDataPacket.destinationNode = 0;
            scheduledDataPacket.messageID = 0;
            return; 
        }
    }
    Serial.println("COLA LLENA: no se pudo encolar dataMessage");
}

inline void enqueueDataMessage(uint32_t payload, uint16_t customDestID) {
    unsigned long randomWait = millis() + random(INITIAL_WAIT_LOWER, INITIAL_WAIT_UPPER);

    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (!scheduledQueue[i].inUse) {
            scheduledQueue[i].isAck   = false;
            scheduledQueue[i].isHello= false;
            scheduledQueue[i].isAlt  = false;

            uint16_t localID = getNodeID();
            uint16_t nextHop = getNextHop(localID, customDestID, 0);

            if (nextHop == INVALID_NEXT_HOP) {
                Serial.printf("enqueueDataMessage => SIN vecinos válidos para destino ");
                return;               
            }

            fillDataPacket(scheduledQueue[i].data,customDestID, nextHop,1, 6, payload);

            scheduledQueue[i].scheduleTime = randomWait;
            scheduledQueue[i].inUse = true;
            return;
        }
    }
    Serial.println("COLA LLENA: no se pudo encolar dataMessage (destino personalizado)");
}

inline void enqueueAckMessage(uint32_t messageID, uint16_t destinationNode) {
    unsigned long randomWait = millis() + random(INITIAL_WAIT_LOWER, INITIAL_WAIT_UPPER);
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (!scheduledQueue[i].inUse) {
            scheduledQueue[i].isAck = true;
            scheduledQueue[i].isHello= false;
            scheduledQueue[i].isAlt  = false;
            fillAckPacket(scheduledQueue[i].ack, messageID, destinationNode);
            scheduledQueue[i].scheduleTime = randomWait;
            scheduledQueue[i].inUse = true;
            return;
        }
    }
    Serial.println("COLA LLENA: no se pudo encolar ACK");
}

inline void enqueueHelloMessage() {
    unsigned long randomWait = millis() + random(INITIAL_WAIT_LOWER, INITIAL_WAIT_UPPER);

    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (!scheduledQueue[i].inUse) {
            scheduledQueue[i].isAck   = false;
            scheduledQueue[i].isHello= true;
            scheduledQueue[i].isAlt  = false;
            fillHelloPacket(scheduledQueue[i].hello);
            scheduledQueue[i].scheduleTime = randomWait;
            scheduledQueue[i].inUse = true;
            return;
        }
    }
    Serial.println("COLA LLENA => No se pudo encolar HELLO");
}
inline void enqueueAltMessage(uint32_t messageID, uint16_t destinationNode) {
    unsigned long randomWait = millis() + random(INITIAL_WAIT_LOWER, INITIAL_WAIT_UPPER);

    for(int i=0; i<MAX_QUEUE_SIZE; i++) {
        if(scheduledQueue[i].inUse == false) {
            scheduledQueue[i].isAck   = false;
            scheduledQueue[i].isHello= false;
            scheduledQueue[i].isAlt  = true;

            fillAltPacket(scheduledQueue[i].alt, messageID, destinationNode);

            scheduledQueue[i].scheduleTime = randomWait;
            scheduledQueue[i].inUse = true;

            Serial.println("ALT encolado en la cola.");
            return;
        }
    }
    Serial.println("COLA LLENA => no se pudo encolar ALT");
}

/*----------------------------------------------------------------------------*/
/*  Envoltorios de “schedule” (interfaz pública)                              */
/*----------------------------------------------------------------------------*/
inline void scheduleAltMessage(uint32_t messageID, uint16_t destinationNode) {
    if (!canSendAlt(messageID)) {
        Serial.printf("ALT SUPRIMIDO para messageID %u (límite %u alcanzado).\n", messageID, ALT_MAX_PER_MESSAGE);
        return;                    
    }
    enqueueAltMessage(messageID, destinationNode);
    Serial.println("ALT programado en cola.");
}

inline void scheduleHelloMessage() {
    enqueueHelloMessage();
}
inline void scheduleMessage(uint32_t payload) {
    enqueueDataMessage(payload);
    Serial.println("Mensaje DATA programado. Esperando tiempo aleatorio en la cola.");
}
inline void scheduleAckMessage(uint32_t messageID, uint16_t destinationNode) {
    enqueueAckMessage(messageID, destinationNode);
    Serial.println("ACK programado. Esperando tiempo aleatorio en la cola.");
}

/*============================================================================*/
/*  5) Inicialización del planificador                                        */
/*============================================================================*/
inline void initMessageScheduler() {
    dataMessageSent = false;
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        scheduledQueue[i].inUse = false;
    }
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        pendingAcks[i].timestamp = 0;
        pendingAcks[i].retryCount = 0;
    }
    scheduledDataPacket.destinationNode = 0;
    scheduledDataPacket.messageID = 0;
    scheduleHelloMessage();
    nextHelloTimeAuto = millis() + HELLO_INTERVAL_MILLIS;
}

/*============================================================================*/
/*  6) Gestion ACKs Pendientes                                                */
/*============================================================================*/
inline void addPendingAck(const DataPacket &packet) {
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (pendingAcks[i].timestamp != 0 && pendingAcks[i].packet.messageID == packet.messageID)
        {
            Serial.printf("[addPendingAck] Ya existe pendiente para messageID=%u, se actualiza.\n",packet.messageID);
            pendingAcks[i].timestamp  = millis();
            return;
        }
    }
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (pendingAcks[i].timestamp == 0) {
            pendingAcks[i].packet = packet;
            pendingAcks[i].timestamp = millis();
            pendingAcks[i].retryCount = 0;
            return;
        }
    }
    Serial.println("No hay espacio en pendingAcks!");
}

/*============================================================================*/
/*  7) HELLO automático                                                       */
/*============================================================================*/
inline void checkAutoHello() {
    if (millis() >= nextHelloTimeAuto) {
        scheduleHelloMessage();
        nextHelloTimeAuto = millis() + HELLO_INTERVAL_MILLIS;
    }
}

/*============================================================================*/
/*  8) Función principal de mantenimiento                                     */
/*============================================================================*/
inline void updateMessageScheduler() {
    /*------ 8.1 Reintentos de ACK -----------------------------------------*/
    for (int i = 0; i < MAX_PENDING_ACKS; i++) {
        if (pendingAcks[i].timestamp != 0 && (millis() - pendingAcks[i].timestamp >= ACK_TIMEOUT)) {
            
            if (pendingAcks[i].retryCount < MAX_RETRIES) {
                Serial.printf("Reintentando envío de messageID: %u\n", pendingAcks[i].packet.messageID);
                scheduledDataPacket = pendingAcks[i].packet; 
                enqueueDataMessage(pendingAcks[i].packet.payload);
                pendingAcks[i].timestamp = millis();
                pendingAcks[i].retryCount++;
            } else {
                Serial.printf("No se recibió ACK tras %u reintentos para messageID: %u. Descarta.\n",MAX_RETRIES, pendingAcks[i].packet.messageID);
                DataPacket original = pendingAcks[i].packet;
                pendingAcks[i].timestamp = 0;
                pendingAcks[i].retryCount = 0;
                reEnqueueAlternateRoute(original, 0, true); 
            }
        }
    }

    /*------ 8.2 Nada que hacer si radio ocupado ---------------------------*/
    if (!loraIdle) {
        return;
    }
    /*------ 8.3 Selección de siguiente elemento listo ---------------------*/
    unsigned long now = millis();
    int indexToSend = -1;
    /* ACK prioritario */
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (scheduledQueue[i].inUse && scheduledQueue[i].scheduleTime <= now && scheduledQueue[i].isAck) {
            indexToSend = i;
            break;
        }
    }
    /* cualquier otro si no hubo ACK listo */
    if (indexToSend == -1) {
        for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
            if (scheduledQueue[i].inUse && scheduledQueue[i].scheduleTime <= now) {
                indexToSend = i;
                break;
            }
        }
    }
    if (indexToSend == -1) {
        return;
    }
    /*------ 8.4 Listen-before-talk ----------------------------------------*/
    windowCollisionPrevention();
    /*------ 8.5 Envío ------------------------------------------------------*/
    if(scheduledQueue[indexToSend].isAlt == true) {
        handleTransmission(scheduledQueue[indexToSend].alt);
        Serial.printf("ALT enviado => messageID=%u\n", scheduledQueue[indexToSend].alt.messageID);
    }
    else if (scheduledQueue[indexToSend].isAck) {
        handleTransmission(scheduledQueue[indexToSend].ack);
        Serial.printf("ACK enviado para messageID: %u\n", scheduledQueue[indexToSend].ack.messageID);
        rememberAckSent(scheduledQueue[indexToSend].ack.messageID);
    } 
    else if (scheduledQueue[indexToSend].isHello) {
        handleTransmission(scheduledQueue[indexToSend].hello);
    }
    else {
        handleTransmission(scheduledQueue[indexToSend].data);
        Serial.printf("Mensaje DATA enviado con payload=%u, nextHop=%u\n",scheduledQueue[indexToSend].data.payload,scheduledQueue[indexToSend].data.nextHop);
        addPendingAck(scheduledQueue[indexToSend].data);
        dataMessageSent = true;
    }
    scheduledQueue[indexToSend].inUse = false;
}

/*============================================================================*/
/*  9) Re-enqueue por ruta alterna                                            */
/*============================================================================*/
inline void reEnqueueAlternateRoute(const DataPacket &originalPacket,uint16_t excludeNeighbor,bool removeNeighborFlag) {

    if (!canReenqueue(originalPacket.messageID)) {
        Serial.printf("reEnqueueAlternateRoute => ""Límite de %u rutas agotado para msgID=%u. Se descarta.\n",ROUTE_MAX_ALTERNATES, originalPacket.messageID);
        return;      
    }
    DataPacket copyPacket = originalPacket;

    if (removeNeighborFlag == true) {
        removeNeighbor(copyPacket.nextHop);
    }

    uint16_t newHop = getNextHop(getNodeID(),copyPacket.destinationNode,excludeNeighbor);

    if (newHop == INVALID_NEXT_HOP) {
        Serial.println("reEnqueueAlternateRoute => No se encontró nextHop, mensaje descartado.");
    } else {
        copyPacket.nextHop = newHop;
        scheduledDataPacket = copyPacket;
        enqueueDataMessage(copyPacket.payload);
        Serial.printf("reEnqueueAlternateRoute => msgID=%u reencolado con nextHop=%u\n",copyPacket.messageID,newHop);
    }
}

/*============================================================================*/
/*  10) Incremento de espera tras recepción                                    */
/*============================================================================*/
inline void increaseWaitTime() {
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        if (scheduledQueue[i].inUse) {
            scheduledQueue[i].scheduleTime += random(BACKOFF_LOWER, BACKOFF_UPPER);
        }
    }
}


#endif
