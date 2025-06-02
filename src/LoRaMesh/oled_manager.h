/*==============================================================================
  oled_conf.h
  ------------------------------------------------------------------------------
  Clase de utilidades para la pantalla SSD1306 64×32.
  – Encendido, apagado, inicialización, mostrar texto y timeout.
==============================================================================*/
#ifndef OLED_CONF_H
#define OLED_CONF_H

#include <Wire.h>
#include "HT_SSD1306Wire.h"

/*----------------------------------------------------------------------------*/
/*  Definiciones de hardware                                                  */
/*----------------------------------------------------------------------------*/
#define OLED_ADDRESS 0x3c // Dirección de la OLED en el bus I2C
#define OLED_FREQUENCY 500000 // Frecuencia de operación
#define SDA_PIN SDA_OLED // Pin SDA para el bus I2C
#define SCL_PIN SCL_OLED // Pin SCL para el bus I2C
#define OLED_GEOMETRY GEOMETRY_64_32 // Resolución de la OLED 64x32 pixels
#define OLED_RESET_PIN RST_OLED // Pin de reinicio de la OLED

/*==============================================================================
  Clase OLEDManager
==============================================================================*/
class OLEDManager {
public:
    OLEDManager() : display(OLED_ADDRESS, OLED_FREQUENCY, SDA_PIN, SCL_PIN, OLED_GEOMETRY, OLED_RESET_PIN) {}
    /* Encendido / apagado de la tensión Vext */
    void oledOn() {
        pinMode(Vext, OUTPUT);
        digitalWrite(Vext, LOW);
    }
    void oledOff() {
        pinMode(Vext, OUTPUT);
        digitalWrite(Vext, HIGH); 
    }
    /* Limpieza total de la pantalla */
    void oledClear() {
        display.clear();  
        display.display();
    }
    /* Muestra mensaje de una sola línea en Y=10 */
    void oledShow(String message) {
        oledClear();  
        display.drawString(0, 10, message);  
        display.display();
    }
    /* Timeout de borrado (OLED_DISPLAY_DURATION) */
    void checkOledTimeout(unsigned long &oled_display_time) {
      if (millis() - oled_display_time >= OLED_DISPLAY_DURATION && oled_display_time != 0) {
          oledClear();  
          oled_display_time = 0; 
      }
    }
    /* Inicialización básica */
    void oledInit() {
        oledOn(); 
        display.init(); 
        display.setFont(ArialMT_Plain_10); 
        display.setTextAlignment(TEXT_ALIGN_LEFT); 
    }

private:
    SSD1306Wire display;
};

#endif
