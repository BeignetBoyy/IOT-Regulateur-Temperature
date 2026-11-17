#include "OneWire.h"
#include "DallasTemperature.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include "AsyncTCP.h"
#include "FS.h"
#include <LittleFS.h>
#include <WiFi.h>

#include "wifi_utils.h"
#include "routes.h"

// --  Defintion des constantes -- //

// Pins :
#define STRIP_PIN 13  // LED strip
#define CLIMPIN   19  // LED climatisation (Bleu)
#define RADPIN    21  // LED radiateur (Rouge)
#define FANPIN    27  // Ventilateur

#define NB_LEDS 5 // Nombres de LED de la LED strip

OneWire oneWire(23); // Pour utiliser une entite oneWire sur le port 23 (Capteur de temperature)
DallasTemperature tempSensor(&oneWire); // Cette entite est utilisee par le capteur de temperature
Adafruit_NeoPixel strip(NB_LEDS, STRIP_PIN, NEO_GRB + NEO_KHZ800); // Utilisé pour la LED Strip

// setup du json
char payload[512];
StaticJsonDocument<512> json;

AsyncWebServer server(80);

// Seuils :
float SH         = 30.00;  // Seuil superieur de temperature (Activation Clim)
float SB         = 28.00;  // Seuil inferieur de temperature (Activation Radiateur)
float SH_VENTILO = 35.00;  // Seuil superieur du ventilateur (Entre 30 et 35°C, la vitesse monte graduellement)
float TEMP_FEU   = 31.00;  // Seuil de temperature pour detection d'incendie
float LUM_FEU    = 300.00; // Seuil de luminosité pour detection d'incendie

void setup(void){
  Serial.begin(9600);

  /* ------------- Setup Wifi et Little FS -------------------*/
  String hostname = "Regulateur de temperature IOT";
  wifi_connect_multi(hostname);               
  
  if (WiFi.status() == WL_CONNECTED){
    Serial.print("\nWiFi connected : yes ! \n"); 
    wifi_printstatus(0);  
  } 
  else {
    Serial.print("\nWiFi connected : no ! \n"); 
    //  ESP.restart();
  }

  if(!LittleFS.begin(true)){
      Serial.println("An Error has occurred while mounting LittleFS");
      return;
  }

  // Setup routes of the ESP Web server
  setup_http_routes(&server);
  
  // Start ESP Web server
  server.begin();

  // ---------------- SETUP CAPTEURS ------------ //

  tempSensor.begin(); // Init capteur temp
  strip.begin(); // Init LED Strip

  pinMode(FANPIN, OUTPUT);  //Init ventilo

  pinMode(CLIMPIN, OUTPUT); //Init led clim
  pinMode(RADPIN, OUTPUT); //Init led radiateur
  pinMode(2, OUTPUT); //Init onboard pin
}

void loop(void){
  float temp;
  int brightness;
  float fan_speed = 0;

  int strip_r = 0;
  int strip_g = 0;
  int strip_b = 0;

  bool clim_led_on = false;
  bool rad_led_on = false;
  bool fire_detected = false;

  // Recuperation des variables depuis Serial in
  updateFromSliders();

  // Capteur luminosité
  brightness = analogRead(A5); //  lecture sur le pin analog 5

  // Capteur temperature
  tempSensor.requestTemperaturesByIndex(0); // Le capteur 0 realise une acquisition
  temp = tempSensor.getTempCByIndex(0); // Recuperation de la temperature en °C

  // -- Gestion des seuils -- //
  
  if(temp > SH){ // Si temperature trop elevé
    //Serial.println("Temperature too high, starting climatisation");
    clim_led_on = true;

    fire_detected = temp > TEMP_FEU && brightness <= LUM_FEU;

    // Gestion ventilo
    /*
    Pour avoir une augmentation graduelle de la vitesse du ventilateur, 
    on a un seuil inferieur et un seuil superieur. Une fois que la temperature a passée le seuil inferieur,
    on commence l'acceleration en prennant la difference entre la temperature actuelle et la borne superieure.
    Puis on converti cette difference en une valeur entre 0 et 255. 
    Au dessus du seuil superieur la vitesse du ventilateur reste constante.
    */
    float fan_range = SH_VENTILO - SH;
    float temp_diff = temp < SH_VENTILO ? SH_VENTILO - temp : 0.0;

    //on inverse la difference puis produit en croix pour obtenir une valeur entre 0 et 255
    fan_speed = ((fan_range - temp_diff) / fan_range) * 255;


    // Couleur de la LED Strip en Rouge 
    strip_r = 255;

  }else if(temp < SB){ // Si temperature trop faible
    //Serial.println("Temperature too low, starting heater");

    rad_led_on = true;

    strip_b = 255;
  }else{
    //Serial.println("Temperature is ideal");
    strip_g = 255;
  }

  //Mise a jour du json
  updateAndSendJson(fire_detected, clim_led_on, rad_led_on, brightness, temp, fan_speed);

  digitalWrite(CLIMPIN, clim_led_on ? HIGH : LOW);
  digitalWrite(FANPIN, rad_led_on ? HIGH : LOW);
  digitalWrite(2, fire_detected ? HIGH : LOW);

  //Serial.printf("FAN SPEED : %f", fan_speed);
  analogWrite(FANPIN, fan_speed);
  
  // Allumage des leds de la Strip
  for(int i=0; i<NB_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(strip_r, strip_g, strip_b));
  }
  strip.show();

  delay(1000); // millis ! attention convertir prend du temps !
}

void updateFromSliders(){
  if (Serial.available() > 0) {
    String incomingString = Serial.readString();
    incomingString.trim(); 

    StaticJsonDocument<256> incoming_json;
    deserializeJson(incoming_json, incomingString);

    // JSON reçu ressemble à ça : {"temp_max":50}
    // Pour le moment parsing pas très propre, a revoir

    String temp_max       = incoming_json["temp_max"];
    String temp_min       = incoming_json["temp_min"];
    String fan_max        = incoming_json["fan_max"];
    String temp_max_fire  = incoming_json["temp_max_fire"];
    String light_max_fire = incoming_json["light_max_fire"];

    if (temp_max != "null") {SH = temp_max.toFloat();}
    if (temp_min != "null") {SB = temp_min.toFloat();}
    if (fan_max != "null") {SH_VENTILO = fan_max.toFloat();}
    if (temp_max_fire != "null") {TEMP_FEU = temp_max_fire.toFloat();}
    if (light_max_fire != "null") {LUM_FEU = light_max_fire.toFloat();}
  }
}

void updateAndSendJson(bool fire_detected, bool clim_led_on, bool rad_led_on, int brightness, float temp, float fan_speed){

  json["status"]["light"] = 4095 - brightness;
  json["status"]["temperature"] = temp;
  json["status"]["fire"] = fire_detected;
  json["status"]["heat"] = rad_led_on ? "ON" : "OFF";
  json["status"]["cold"] = clim_led_on ? "ON" : "OFF";
  json["status"]["fanspeed"] = fan_speed;
  json["status"]["regul"] = "NOP";

  /*Reste des variables du json
  Pour le moment les valeurs sont nulles*/
  json["location"]["room"] = "NOP";
  json["location"]["gps"]["lat"] = 0.0;
  json["location"]["gps"]["lon"] = 0.0;
  json["location"]["address"] = "NOP";

  json["regul"]["lt"] = SB;
  json["regul"]["ht"] = SH;

  json["info"]["indent"] = "NOP";
  json["info"]["user"] = "NOP";
  json["info"]["loc"] = "NOP";

  json["net"]["uptime"] = "NOP";
  json["net"]["ssid"] = WiFi.SSID();
  json["net"]["mac"] = WiFi.macAddress();
  json["net"]["ip"] = WiFi.localIP().toString();

  json["reporthost"]["target_ip"] = "NOP";
  json["reporthost"]["target_port"] = 0;
  json["reporthost"]["sp"] = 0;

  serializeJson(json, payload);
  //Serial.println(payload);
}