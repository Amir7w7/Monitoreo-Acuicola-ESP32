#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFiClientSecure.h>

const char* ssid="TU_RED_WIFI";
const char* password="TU_PASSWORD_WIFI";
const char* urlMakeCom="https://hook.REGION.make.com/TU_WEBHOOK";

const int PIN_BOTON=33, LED_VERDE=14, LED_AMARILLO=25, LED_ROJO=13;
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH,SCREEN_HEIGHT,&Wire,-1);
bool oledDisponible=false;
int estadoPantalla=0,estadoBotonActual=HIGH,estadoBotonUltimo=HIGH;
unsigned long ultimoTiempoRebote=0,ultimoTiempoBoton=0;
const long tiempoApagadoPantalla=60000;
bool pantallaEncendida=true;

typedef struct paquete_datos {float temp; float ph; float ntu;} paquete_datos;
paquete_datos datosRecibidos;
volatile bool nuevosDatos=false;

void actualizarSemaforo(float ph,float ntu){
 digitalWrite(LED_VERDE,LOW); digitalWrite(LED_AMARILLO,LOW); digitalWrite(LED_ROJO,LOW);
 bool ntuOptimo=(ntu>=5.0&&ntu<=25.0), ntuAlerta=(ntu>25.0&&ntu<=40.0), ntuCritico=(ntu>40.0||ntu<5.0);
 bool phOptimo=(ph>=6.8&&ph<=8.2), phAlerta=(ph>=6.0&&ph<=9.0);
 if(ntuCritico||!phAlerta){digitalWrite(LED_ROJO,HIGH); Serial.println("ESTADO: ¡PELIGRO! Riesgo para peces.");}
 else if(ntuAlerta||!phOptimo){digitalWrite(LED_AMARILLO,HIGH); Serial.println("ESTADO: ALERTA TEMPRANA.");}
 else if(ntuOptimo&&phOptimo){digitalWrite(LED_VERDE,HIGH); Serial.println("ESTADO: ÓPTIMO.");}
}

void actualizarPantalla(){
 if(!oledDisponible||!pantallaEncendida)return;
 display.clearDisplay(); display.setTextSize(1); display.setTextColor(WHITE); display.setCursor(0,0);
 if(estadoPantalla==0){display.println("--- PISCINA ---"); display.print("Temp: ");display.print(datosRecibidos.temp);display.println(" C");display.print("pH:   ");display.println(datosRecibidos.ph);display.print("NTU:  ");display.println(datosRecibidos.ntu);}
 else if(estadoPantalla==1){display.setTextSize(2);display.println("TEMPERATURA");display.print(datosRecibidos.temp);display.println(" C");}
 else if(estadoPantalla==2){display.setTextSize(2);display.println("NIVEL pH");display.print(datosRecibidos.ph);}
 else if(estadoPantalla==3){display.setTextSize(2);display.println("TURBIDEZ");display.print(datosRecibidos.ntu);display.println(" NTU");}
 display.display();
}

void enviarAMake(){
 if(WiFi.status()==WL_CONNECTED){
  float tempVal=isnan(datosRecibidos.temp)?0.0:datosRecibidos.temp;
  float phVal=isnan(datosRecibidos.ph)?0.0:datosRecibidos.ph;
  float ntuVal=isnan(datosRecibidos.ntu)?0.0:datosRecibidos.ntu;
  String jsonPost="{\"temp\":"+String(tempVal,2)+",\"ph\":"+String(phVal,2)+",\"ntu\":"+String(ntuVal,2)+"}";
  Serial.print("Enviando JSON a Make: ");Serial.println(jsonPost);
  WiFiClientSecure client; client.setInsecure();
  HTTPClient http; http.begin(client,urlMakeCom); http.addHeader("Content-Type","application/json"); http.addHeader("User-Agent","ESP32");
  int code=http.POST(jsonPost); Serial.printf("Respuesta HTTP: %d\n",code); http.end();
 } else {Serial.println("Wi-Fi no disponible. Intentando reconectar..."); WiFi.reconnect();}
}

void OnDataRecv(const uint8_t *mac,const uint8_t *incomingData,int len){
 if(len!=sizeof(datosRecibidos)){Serial.printf("Paquete incorrecto: %d bytes; esperados: %d\n",len,sizeof(datosRecibidos));return;}
 memcpy(&datosRecibidos,incomingData,sizeof(datosRecibidos)); nuevosDatos=true;
}

void setup(){
 Serial.begin(115200);
 pinMode(PIN_BOTON,INPUT_PULLUP); pinMode(LED_VERDE,OUTPUT); pinMode(LED_AMARILLO,OUTPUT); pinMode(LED_ROJO,OUTPUT);
 digitalWrite(LED_VERDE,LOW);digitalWrite(LED_AMARILLO,LOW);digitalWrite(LED_ROJO,LOW);
 if(display.begin(SSD1306_SWITCHCAPVCC,0x3C)){oledDisponible=true;display.clearDisplay();display.setTextColor(WHITE);display.setCursor(0,10);display.println("Iniciando...");display.display();}
 WiFi.mode(WIFI_STA); WiFi.begin(ssid,password); Serial.print("Conectando a WiFi");
 int intentos=0; while(WiFi.status()!=WL_CONNECTED&&intentos<30){delay(300);Serial.print(".");intentos++;}
 if(WiFi.status()==WL_CONNECTED){Serial.println("\nWiFi Conectado!");Serial.print("MAC del Maestro: ");Serial.println(WiFi.macAddress());Serial.print("Canal WiFi del Router: ");Serial.println(WiFi.channel());esp_wifi_set_ps(WIFI_PS_MIN_MODEM);} else Serial.println("\nError al conectar WiFi.");
 if(esp_now_init()==ESP_OK){esp_now_register_recv_cb(OnDataRecv);Serial.println("ESP-NOW listo.");} else Serial.println("ERROR: No se pudo iniciar ESP-NOW.");
 if(oledDisponible){display.clearDisplay();display.setCursor(0,10);display.println("Esperando a la boya...");display.display();}
 ultimoTiempoBoton=millis();
}

void loop(){
 unsigned long tiempoActual=millis();
 int lecturaBoton=digitalRead(PIN_BOTON);
 if(lecturaBoton!=estadoBotonUltimo)ultimoTiempoRebote=tiempoActual;
 if((tiempoActual-ultimoTiempoRebote)>50&&lecturaBoton!=estadoBotonActual){
  estadoBotonActual=lecturaBoton;
  if(estadoBotonActual==LOW){ultimoTiempoBoton=tiempoActual;if(!pantallaEncendida&&oledDisponible){display.ssd1306_command(SSD1306_DISPLAYON);pantallaEncendida=true;actualizarPantalla();}else if(oledDisponible){estadoPantalla++;if(estadoPantalla>3)estadoPantalla=0;actualizarPantalla();}}
 }
 estadoBotonUltimo=lecturaBoton;
 if(pantallaEncendida&&oledDisponible&&(tiempoActual-ultimoTiempoBoton>=tiempoApagadoPantalla)){display.ssd1306_command(SSD1306_DISPLAYOFF);pantallaEncendida=false;}
 if(nuevosDatos){nuevosDatos=false;Serial.println("\n¡Dato recibido de la boya!");Serial.print("Temperatura: ");Serial.println(datosRecibidos.temp);Serial.print("pH: ");Serial.println(datosRecibidos.ph);Serial.print("NTU: ");Serial.println(datosRecibidos.ntu);actualizarSemaforo(datosRecibidos.ph,datosRecibidos.ntu);actualizarPantalla();enviarAMake();}
 delay(10);
}
