/*==============================================================================
  routing_manager.h
  ------------------------------------------------------------------------------
  Mantenimiento de tabla de vecinos y selección de nextHop:
  – Guarda RSSI y marca temporal por cada vecino que responde HELLO.
  – Elimina vecinos inactivos.
  – Selecciona nextHop con métrica compuesta RSSI + antigüedad.
==============================================================================*/
#ifndef ROUTING_MANAGER_H
#define ROUTING_MANAGER_H

#include "config.h"
#include "packet_manager.h"


/*----------------------------------------------------------------------------*/
/*  Tabla de vecinos                                                          */
/*----------------------------------------------------------------------------*/
struct NeighborInfo {
  uint16_t neighborId;
  int16_t  rssi;
  unsigned long lastHeard; 
};
static NeighborInfo neighborTable[MAX_NEIGHBORS];

/*----------------------------------------------------------------------------*/
/*  Lista blanca opcional (ALLOWED_NEIGHBORS)                                 */
/*----------------------------------------------------------------------------*/
inline bool isAllowedNeighbor(uint16_t nodeID) {
    static uint16_t allowed[] = ALLOWED_NEIGHBORS;
    if (allowed[0] == 0) { // sin filtro
        return true; 
    }
    for (int i = 0; allowed[i] != 0; i++) {
        if (allowed[i] == nodeID) {
            return true;
        }
    }
    return false;
}
/*----------------------------------------------------------------------------*/
/*  Operaciones sobre la tabla                                                */
/*----------------------------------------------------------------------------*/
inline void addOrUpdateNeighbor(uint16_t neighborId, int16_t rssi) {
    // Buscar existente
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId == neighborId) {
            neighborTable[i].rssi = rssi;
            neighborTable[i].lastHeard = millis();
            return;
        }
    }
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId == 0) {
            neighborTable[i].neighborId = neighborId;
            neighborTable[i].rssi       = rssi;
            neighborTable[i].lastHeard = millis();
            return;
        }
    }
    Serial.println("Tabla de vecinos llena => no se pudo agregar.");
}

inline void cleanupNeighbors() {
    unsigned long now = millis();
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId != 0) {
            if ((now - neighborTable[i].lastHeard) > NEIGHBOR_EXPIRATION_TIME) {
                Serial.printf("Eliminando vecino %u por inactividad.\n", neighborTable[i].neighborId);
                neighborTable[i].neighborId = 0;
                neighborTable[i].rssi = 0;
                neighborTable[i].lastHeard = 0;
            }
        }
    }
}


inline void removeNeighbor(uint16_t neighborId) {
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId == neighborId) {
            Serial.printf("Eliminando vecino %u (por ACK no recibido o similar).\n", neighborId);
            neighborTable[i].neighborId = 0;
            neighborTable[i].rssi = 0;
            neighborTable[i].lastHeard = 0;
            return;
        }
    }
}

/*----------------------------------------------------------------------------*/
/*  Impresión de la tabla                                                     */
/*----------------------------------------------------------------------------*/
inline void printNeighborTable() {
    Serial.println("=== Tabla de Vecinos ===");
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId != 0) {
            Serial.printf("  Vecino: %u, RSSI: %d, lastHeard: %lu\n",neighborTable[i].neighborId,neighborTable[i].rssi,neighborTable[i].lastHeard);
        }
    }
    Serial.println("========================");
}

/*============================================================================*/
/*  Métrica y algoritmo de selección de nextHop                               */
/*============================================================================*/
inline float getNeighborScore(int16_t rssi, unsigned long lastHeard) {
    unsigned long now = millis();
    float secsAgo = (float)(now - lastHeard) / 1000.0;
    return (float)rssi - secsAgo;
}

inline uint16_t getNextHop(uint16_t localID, uint16_t destID, uint16_t excludeID) {
    /*-------------------------------- Destino directo ---------------------*/
    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        if (neighborTable[i].neighborId == destID) {
            return destID;
        }
    }
    /*------------------------------- Candidatos ---------------------------*/
    struct Candidate {
        uint16_t id;
        float score;
    };
    Candidate allCandidates[MAX_NEIGHBORS];
    int countAll = 0;

    for (int i = 0; i < MAX_NEIGHBORS; i++) {
        uint16_t neighborIdCandidate = neighborTable[i].neighborId;
        if (neighborIdCandidate == 0) {
            continue; 
        }
        if (neighborIdCandidate == localID) {
            continue; 
        }
        if (neighborIdCandidate == excludeID) {
            continue; 
        }
        if (!isAllowedNeighbor(neighborIdCandidate)) {
            continue; 
        }

        float sc = getNeighborScore(neighborTable[i].rssi, neighborTable[i].lastHeard);
        allCandidates[countAll].id = neighborIdCandidate;
        allCandidates[countAll].score = sc;
        countAll++;
    }
    if (countAll == 0) {
        Serial.println("getNextHop => NO vecinos válidos");
        return INVALID_NEXT_HOP;
    }
    /*------------------------------- Ordena por score ---------------------*/
    for (int outer = 0; outer < (countAll - 1); outer++) {
        for (int inner = 0; inner < (countAll - 1 - outer); inner++) {
            if (allCandidates[inner].score < allCandidates[inner + 1].score) {
                Candidate temp = allCandidates[inner];
                allCandidates[inner] = allCandidates[inner + 1];
                allCandidates[inner + 1] = temp;
            }
        }
    }
    int topCount = countAll;
    if (topCount > ROUTING_MAX_CANDIDATES) {
        topCount = ROUTING_MAX_CANDIDATES;
    }
    if (topCount > 0) {
        int chosenIndex = random(0, topCount);
        uint16_t chosenId = allCandidates[chosenIndex].id;
        float chosenScore = allCandidates[chosenIndex].score;
        Serial.printf("getNextHop => TopCount=%d, elegido %u con score=%.2f\n",topCount, chosenId, chosenScore);
        return chosenId;
    }
    if (countAll > 0) {
        Serial.println("getNextHop => topCount=0, usando primer candidato disponible...");
        return allCandidates[0].id; 
    }
    Serial.println("getNextHop => Sin candidatos => retorno ID error");
    return INVALID_NEXT_HOP;
}

#endif
