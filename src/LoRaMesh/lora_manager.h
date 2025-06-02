/*==============================================================================
  lora_manager.h
  ------------------------------------------------------------------------------
  Envoltura mínima sobre el driver oficial Heltec SX1262.
  – Inicializa el módulo LoRa y asocia la tabla de callbacks RadioEvents_t.
  – Expone utilidades de configuración para recepción (RX) y transmisión (TX).
  – Proporciona accesos directos a las funciones esenciales del driver
    (receive, send, processIrq y sleep).
==============================================================================*/
#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include "LoRaWan_APP.h"
#include "Arduino.h"

/*==============================================================================
  Clase LoraManager
==============================================================================*/
class LoraManager {
public:
    /*----------------------------------------------------------------------------*/
    /*  initLoRa()                                                                */
    /*----------------------------------------------------------------------------*/
    /*  – Inicializa el microcontrolador Heltec (Mcu.begin).                      */
    /*  – Registra la tabla de eventos recibida por parámetro.                    */
    /*  – Fija la frecuencia (canal) de operación.                                */
    /*----------------------------------------------------------------------------*/
    void initLoRa(RadioEvents_t *radioEvents, uint32_t frequency) {
        Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
        Radio.Init(radioEvents);
        Radio.SetChannel(frequency);
    }
    /*----------------------------------------------------------------------------*/
    /*  setRxConfig()                                                             */
    /*----------------------------------------------------------------------------*/
    /*  Configura los parámetros de recepción.                                    */
    /*  - bandwidth ....... Ancho de banda (0 = 125 kHz, 1 = 250 kHz, etc.).      */
    /*  - sf .............. Spreading Factor (7-12).                              */
    /*  - cr .............. Coding Rate (1 = 4/5, 2 = 4/6, …).                    */
    /*  - preamble ........ Longitud de preámbulo en símbolos.                    */
    /*  - symbolTimeout ... Límite de símbolos sin cabecera (timeout).            */
    /*  - fixLen .......... true ⇒ payload de longitud fija.                      */
    /*  - iqInv ........... true ⇒ inversión I/Q (LoRaWAN).                       */
    /*----------------------------------------------------------------------------*/
    void setRxConfig(uint32_t bandwidth, uint32_t spreadingFactor, uint8_t codingRate,
                     uint16_t preambleLength, uint16_t symbolTimeout,
                     bool fixLengthPayload, bool iqInversion) {
        Radio.SetRxConfig(MODEM_LORA, bandwidth, spreadingFactor, codingRate, 0, preambleLength,
                          symbolTimeout, fixLengthPayload, 0, true, 0, 0, iqInversion, true);
    }
    /*----------------------------------------------------------------------------*/
    /*  setTxConfig()                                                             */
    /*----------------------------------------------------------------------------*/
    /*  Configura los parámetros de transmisión.                                  */
    /*  - power ............ Potencia de salida en dBm.                           */
    /*  - bandwidth ........ Ancho de banda (0 = 125 kHz, 1 = 250 kHz, etc.).     */
    /*  - sf ............... Spreading Factor.                                    */
    /*  - cr ............... Coding Rate.                                         */
    /*  Resto de argumentos permanecen con valores por defecto del driver.        */
    /*----------------------------------------------------------------------------*/
    void setTxConfig(int8_t power, uint32_t bandwidth, uint8_t spreadingFactor, uint8_t codingRate) {
        Radio.SetTxConfig(MODEM_LORA, power, 0, bandwidth, spreadingFactor, codingRate, 8, false, true, 0, 0, false, 3000);
    }

    /*----------------------------------------------------------------------------*/
    /*  Accesos directos al driver                                                */
    /*----------------------------------------------------------------------------*/
    void processIrq() {
        Radio.IrqProcess();
    }
    void receive() {
        Radio.Rx(0);
    }
    void send(uint8_t *buffer, uint16_t size) {
        Radio.Send(buffer, size);
    }
    void sleep() {
        Radio.Sleep();
    }


    
};

#endif