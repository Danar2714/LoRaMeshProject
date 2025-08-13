/*==============================================================================
  config.h
  ------------------------------------------------------------------------------
  Parámetros de compilación y constantes de la red LoRa Mesh.
  – Frecuencia, SF, BW, timeout de ACK, tamaños de buffer, etc.
  – Todos los comentarios de “opción” se conservan.
==============================================================================*/
#ifndef CONFIG_H
#define CONFIG_H


/*----------------------------------------------------------------------------*/
/*  Radio SX1262 – parámetros básicos                                         */
/*----------------------------------------------------------------------------*/
#define RF_FREQUENCY 915000000 // Hz
#define TX_OUTPUT_POWER 22      // dBm
#define LORA_BANDWIDTH 0       // 125 kHz
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1      // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

/*----------------------------------------------------------------------------*/
/*  Buffers                                                                   */
/*----------------------------------------------------------------------------*/
#define BUFFER_SIZE 30
#define MAX_PACKET_SIZE 256 

/*----------------------------------------------------------------------------*/
/*  Identificación de malla                                                   */
/*----------------------------------------------------------------------------*/
#define MESH_ID 0x1234

/*----------------------------------------------------------------------------*/
/*  OLED                                                                      */
/*----------------------------------------------------------------------------*/
#define OLED_DISPLAY_DURATION 5000 // 5 segundos

/*----------------------------------------------------------------------------*/
/*  Temporizadores de envío y back-off                                        */
/*----------------------------------------------------------------------------*/
#define INITIAL_WAIT_LOWER 3000  // 3 segundos
#define INITIAL_WAIT_UPPER 7000  // 7 segundos
#define BACKOFF_LOWER 500   // 500 milisegundos
#define BACKOFF_UPPER 1000  // 1 segundo

/*----------------------------------------------------------------------------*/
/*  Tipos de paquete                                                          */
/*----------------------------------------------------------------------------*/
#define MESSAGE_TYPE_DATA 1
#define MESSAGE_TYPE_ACK 2
#define MESSAGE_TYPE_HELLO  3
#define MESSAGE_TYPE_ALT 4 

/*----------------------------------------------------------------------------*/
/*  ACK y reintentos                                                          */
/*----------------------------------------------------------------------------*/
#define MAX_PENDING_ACKS 10
#define ACK_TIMEOUT 15000  // 15 segundos
#define ACK_REPLAY_WINDOW   10       
#define ACK_REPLAY_TTL_MS   15000   
#define MAX_RETRIES 3

/*----------------------------------------------------------------------------*/
/*  Cola y LBT                                                                */
/*----------------------------------------------------------------------------*/
#define MAX_QUEUE_SIZE 10
#define LISTEN_WINDOW_MS 500 //ms
#define MAX_WINDOW_RETRIES 5

/*----------------------------------------------------------------------------*/
/*  Vecinos y enrutamiento                                                    */
/*----------------------------------------------------------------------------*/
#define MAX_NEIGHBORS 10
#define HELLO_INTERVAL_MILLIS 60000
#define NEIGHBOR_EXPIRATION_TIME 120000
#define ROUTING_MAX_CANDIDATES 3
#define INVALID_NEXT_HOP 0xFFFF

/* Lista de vecinos permitidos (0 ⇒ sin filtro) */
//#define ALLOWED_NEIGHBORS {10412, 0 } //Para (A-liga-extremo)
#define ALLOWED_NEIGHBORS {33364,2289,61039, 0 } //Para (B-normal)
//#define ALLOWED_NEIGHBORS {10412,21087, 0 } //Para (C-bruja-intermedio) y (D-raya-intermedio)
//#define ALLOWED_NEIGHBORS {2289,61039, 0 } //Para (E-cruz-extremo)
//#define ALLOWED_NEIGHBORS { 0 }

/*----------------------------------------------------------------------------*/
/*  Control de re-enqueue y ALT                                               */
/*----------------------------------------------------------------------------*/
#define ROUTE_MAX_ALTERNATES   5     
#define ROUTE_HISTORY_SIZE     10    
#define MAX_DUPLICATE_HISTORY 30
#define ALT_MAX_PER_MESSAGE   1      
#define ALT_HISTORY_SIZE     30      

#endif
