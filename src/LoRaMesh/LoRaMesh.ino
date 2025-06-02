/*==============================================================================
  LoRaMesh.ino
  ------------------------------------------------------------------------------
  Punto de arranque del prototipo LoRa Mesh basado en Heltec Wireless Stick V3.
  – Inicializa Serial, radio LoRa (SX1262), OLED y subsistemas auxiliares.
  – Atiende comandos por consola para enviar DATA, HELLO o mostrar vecinos.
  – Mantiene la recepción continua y la ejecución del planificador de mensajes.
==============================================================================*/
#include "config.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"
#include "oled_manager.h"
#include "lora_manager.h"
#include "packet_manager.h"
#include "communication_manager.h"
#include "message_scheduler.h"
#include "message_receiver.h"
#include "routing_manager.h"  

/*----------------------------------------------------------------------------*/
/*  Variables de estado global                                                */
/*----------------------------------------------------------------------------*/
volatile bool loraIdle = true;
volatile bool transmissionDone = false;
volatile bool receptionDone = false;
volatile bool transmissionError = false;

/*----------------------------------------------------------------------------*/
/*  Buffer de recepción y metadatos                                           */
/*----------------------------------------------------------------------------*/
uint8_t receivedBuffer[MAX_PACKET_SIZE];
uint16_t receivedSize = 0;
int16_t receivedRssi = 0;
int8_t receivedSnr = 0;

/*----------------------------------------------------------------------------*/
/*  Objetos de apoyo                                                          */
/*----------------------------------------------------------------------------*/
OLEDManager oledDisplay; // manejo de pantalla OLED
static RadioEvents_t RadioEvents; // callbacks SX1262
LoraManager loraAntena; // abstracción de funciones LoRa

/*  Paquete deserializado global (se usa para imprimir en varias rutinas)     */
DataPacket receivedPacket;   


/*----------------------------------------------------------------------------*/
/*  Variables auxiliares para la aplicación                                   */
/*----------------------------------------------------------------------------*/
bool dataMessageSent = false;
int payloadCounter = 1;
unsigned long oledDisplayTime = 0;

/*============================================================================*/
/*  setup(): configuración inicial                                            */
/*============================================================================*/
void setup() {
  
  Serial.begin(115200);
  randomSeed(analogRead(0)); // semilla pseudoaleatoria

  /*-- Registro de callbacks e inicio del radio ---------------------------*/
  initTxRxEvents(RadioEvents); 
  loraAntena.initLoRa(&RadioEvents, RF_FREQUENCY);
  loraAntena.setTxConfig(TX_OUTPUT_POWER, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE);
  loraAntena.setRxConfig(LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT, false, false);
  /*-- OLED ---------------------------------------------------------------*/
  oledDisplay.oledOn();
  oledDisplay.oledInit();
  Serial.printf("Node ID: %u\n", getNodeID());
  Serial.println("Comandos disponibles:");
  Serial.println("  'nodeID' => Enviar Data con contador");
  Serial.println("  'h' => Enviar Hello");
  Serial.println("  'v' => Mostrar tabla de vecinos");

  /*-- Subsistemas --------------------------------------------------------*/
  initMessageScheduler();
  initMessageReceiver();
}

/*============================================================================*/
/*  loop(): bucle principal                                                   */
/*============================================================================*/
void loop() {
  /*--------------------------- Consola -----------------------------------*/
  if (Serial.available() > 0) {
    char input = Serial.read();  
    if (input == 'h' && loraIdle) { // HELLO manual
      scheduleHelloMessage();
    }
    else if (input == 'v') { // tabla de vecinos
      printNeighborTable();
    }
    else if (isdigit(input)) { // destino
      String numericStr;
      numericStr += input;
      while (Serial.available() > 0) {
        char c = Serial.peek();
        if (!isdigit(c)) {
          break;
        }
        numericStr += (char) Serial.read();
      }
      uint16_t nodeID = numericStr.toInt();
      if (loraIdle && nodeID > 0) {
        enqueueDataMessage(payloadCounter, nodeID);
      }
    }

  }
  /*------------------ Recepción pasiva y procesamiento -------------------*/
  if (loraIdle) {
    handleReception();  // escucha continua
  }
   uint8_t receivedType = processReceivedMessage(oledDisplayTime); // Procesar mensajes recibidos
  if (receivedType == MESSAGE_TYPE_DATA) {
      oledDisplay.oledClear();
      oledDisplay.oledShow(String("Recibido: ") + String(receivedPacket.payload));
  }

  /*---------------- Planificador, HELLO auto ------------------*/
  updateMessageScheduler(); // cola de envíos y reintentos
  checkAutoHello(); // HELLO periódico
  loraAntena.processIrq(); // atiende IRQs del SX1262

  /*-------------------- Gestión de eventos TX ----------------------------*/
 if (transmissionDone) {
        if (dataMessageSent) {
            oledDisplay.oledClear();
            oledDisplay.oledShow(String("Enviado: ") + String(payloadCounter));
            oledDisplayTime = millis();
            payloadCounter++;
            dataMessageSent = false;
        }

        transmissionDone = false;
    }
  if (transmissionError) {
    Serial.println("Error de transmisión.");
    oledDisplay.oledClear();
    oledDisplay.oledShow(String("Error envio: ") + String(payloadCounter)); 
    oledDisplayTime = millis();
    transmissionError = false; 
  }
  /*------------------- Refresco OLED (timeout) ---------------------------*/
  oledDisplay.checkOledTimeout(oledDisplayTime);

  /*---------------- Limpieza de vecinos-----------------------------------*/
  cleanupNeighbors();
}

