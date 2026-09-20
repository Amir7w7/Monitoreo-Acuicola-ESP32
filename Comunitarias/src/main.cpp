#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <OneWire.h>
#include <DallasTemperature.h>

uint8_t macMaestro[] = {0x00,0x00,0x00,0x00,0x00,0x00};
#define CANAL_WIFI 11
unsigned long ultimoEnvio=0;
const unsigned long intervaloEnvio=10000;
const int PIN_DS18B20=4;
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensorTemp(&oneWire);
const int PIN_PH=34;
const int PIN_TURBIDEZ=35;

typedef struct paquete_datos { float temp; float ph; float ntu; } paquete_datos;
paquete_datos misDatos;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
 Serial.print("ESP-NOW -> ");
 Serial.println(status==ESP_NOW_SEND_SUCCESS?"ENTREGA EXITOSA":"FALLO DE ENTREGA");
}

float leerTemperatura(){
 sensorTemp.requestTemperatures();
 float tempC=sensorTemp.getTempCByIndex(0);
 Serial.print("DS18B20 -> ");
 if(tempC==DEVICE_DISCONNECTED_C || tempC < -50.0){Serial.println("ERROR / SENSOR DESCONECTADO"); return 0.0;}
 Serial.print(tempC); Serial.println(" °C"); return tempC;
}

float leerPH(){
 long sumaADC=0;
 for(int i=0;i<10;i++){sumaADC+=analogRead(PIN_PH); delay(10);}
 float adcPromedio=sumaADC/10.0;
 float voltaje=(adcPromedio/4095.0)*3.3;
 float valorPH=7.0+((2.5-voltaje)/0.18);
 Serial.print("pH -> ADC: "); Serial.print(adcPromedio);
 Serial.print(" | Voltaje: "); Serial.print(voltaje,3);
 Serial.print(" V | pH: "); Serial.println(valorPH,2);
 return valorPH;
}

float leerTurbidez(){
 long sumaADC=0;
 for(int i=0;i<30;i++){sumaADC+=analogRead(PIN_TURBIDEZ); delay(5);}
 float adcPromedio=sumaADC/30.0;
 float voltaje=(adcPromedio/4095.0)*3.3;
 Serial.print("TURBIDEZ -> ADC: "); Serial.print(adcPromedio);
 Serial.print(" | Voltaje: "); Serial.print(voltaje,3); Serial.println(" V");
 float voltajeAguaLimpia=2.02, voltajeAguaTurbia=0.50, valorNTU;
 if(voltaje>=voltajeAguaLimpia) valorNTU=0.0;
 else if(voltaje<=voltajeAguaTurbia) valorNTU=3000.0;
 else valorNTU=(voltajeAguaLimpia-voltaje)*(3000.0/(voltajeAguaLimpia-voltajeAguaTurbia));
 Serial.print("NTU calculado: "); Serial.println(valorNTU,2);
 return valorNTU;
}

void setup(){
 Serial.begin(115200); delay(2000);
 Serial.println("\n==============================\n       BOYA ESP32\n==============================");
 analogSetAttenuation(ADC_11db);
 sensorTemp.begin();
 Serial.print("DS18B20 encontrados: "); Serial.println(sensorTemp.getDeviceCount());
 WiFi.mode(WIFI_STA); WiFi.disconnect(); delay(100);
 Serial.print("MAC DE LA BOYA: "); Serial.println(WiFi.macAddress());
 esp_wifi_set_promiscuous(true);
 esp_wifi_set_channel(CANAL_WIFI,WIFI_SECOND_CHAN_NONE);
 esp_wifi_set_promiscuous(false);
 if(esp_now_init()!=ESP_OK){Serial.println("ERROR: No se pudo iniciar ESP-NOW"); return;}
 esp_now_register_send_cb(OnDataSent);
 esp_now_peer_info_t peerInfo={};
 memcpy(peerInfo.peer_addr,macMaestro,6);
 peerInfo.channel=CANAL_WIFI; peerInfo.encrypt=false;
 if(esp_now_add_peer(&peerInfo)!=ESP_OK){Serial.println("ERROR: No se pudo agregar al Maestro"); return;}
 misDatos.temp=leerTemperatura(); misDatos.ph=leerPH(); misDatos.ntu=leerTurbidez();
 esp_err_t resultado=esp_now_send(macMaestro,(uint8_t*)&misDatos,sizeof(misDatos));
 Serial.print("Resultado esp_now_send(): "); Serial.println(resultado);
 ultimoEnvio=millis();
}

void loop(){
 unsigned long tiempoActual=millis();
 // Valores de prueba presentes en la versión entregada; las lecturas reales los reemplazan al cumplirse el intervalo.
 misDatos.temp=25.50; misDatos.ph=7.20; misDatos.ntu=15.00;
 if(tiempoActual-ultimoEnvio>=intervaloEnvio){
  ultimoEnvio=tiempoActual;
  misDatos.temp=leerTemperatura(); misDatos.ph=leerPH(); misDatos.ntu=leerTurbidez();
  Serial.printf("Temp: %.2f °C\npH: %.2f\nNTU: %.2f\n",misDatos.temp,misDatos.ph,misDatos.ntu);
  esp_err_t resultado=esp_now_send(macMaestro,(uint8_t*)&misDatos,sizeof(misDatos));
  Serial.print("Resultado esp_now_send(): "); Serial.println(resultado);
 }
 delay(10);
}
