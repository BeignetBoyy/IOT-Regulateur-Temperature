#include "OneWire.h"
#include "DallasTemperature.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// --  Defintion des constantes -- //

// Pins :
#define STRIP_PIN 13  // LED strip
#define CLIMPIN   19  // LED climatisation (Bleu)
#define RADPIN    21  // LED radiateur (Rouge)
#define FANPIN    27  // Ventilateur

// Seuils :
#define SH          30.00 // Seuil superieur de temperature (Activation Clim)
#define SB          28.00 // Seuil inferieur de temperature (Activation Radiateur)
#define SH_VENTILO  35.00 // Seuil superieur du ventilateur (Entre 30 et 35°C, la vitesse monte graduellement)
#define TEMP_FEU    31.00 // Seuil de temperature pour detection d'incendie
#define LUM_FEU     300   // Seuil de luminosité pour detection d'incendie

#define NB_LEDS 5 // Nombres de LED de la LED strip

OneWire oneWire(23); // Pour utiliser une entite oneWire sur le port 23 (Capteur de temperature)
DallasTemperature tempSensor(&oneWire); // Cette entite est utilisee par le capteur de temperature
Adafruit_NeoPixel strip(NB_LEDS, STRIP_PIN, NEO_GRB + NEO_KHZ800); // Utilisé pour la LED Strip

void setup(void){
  Serial.begin(9600);
  tempSensor.begin(); // Init capteur temp
  strip.begin(); // Init LED Strip

  pinMode(FANPIN, OUTPUT);  //Init ventilo

  pinMode(CLIMPIN, OUTPUT); //Init led clim
  pinMode(RADPIN, OUTPUT); //Init led radiateur
  pinMode(2, OUTPUT); //Init onboard pin
}

void loop(void){
  float t;
  int brightness;
  float fan_speed = 0;

  int strip_r = 0;
  int strip_g = 0;
  int strip_b = 0;

  bool clim_led_on = false;
  bool rad_led_on = false;
  bool fire_detected = false;

  // setup du json
  char payload[256];
  StaticJsonDocument<256> jdoc;

  // Capteur luminosité
  brightness = analogRead(A5); //  lecture sur le pin analog 5
  jdoc["status"]["light"] = 4095 - brightness;

  // Capteur temperature
  tempSensor.requestTemperaturesByIndex(0); // Le capteur 0 realise une acquisition
  t = tempSensor.getTempCByIndex(0); // Recuperation de la temperature en °C
  jdoc["status"]["temperature"] = t;

  // -- Gestion des seuils -- //
  
  if(t > SH){ // Si temperature trop elevé
    //Serial.println("Temperature too high, starting climatisation");
    clim_led_on = true;

    fire_detected = t > TEMP_FEU && brightness <= LUM_FEU;

    // Gestion ventilo
    /*
    Pour avoir une augmentation graduelle de la vitesse du ventilateur, 
    on a un seuil inferieur et un seuil superieur. Une fois que la temperature a passée le seuil inferieur,
    on commence l'acceleration en prennant la difference entre la temperature actuelle et la borne superieure.
    Puis on converti cette difference en une valeur entre 0 et 255. 
    Au dessus du seuil superieur la vitesse du ventilateur reste constante.
    */
    float fan_range = SH_VENTILO - SH;
    float temp_diff = t < SH_VENTILO ? SH_VENTILO - t : 0.0;

    //on inverse la difference puis produit en croix pour obtenir une valeur entre 0 et 255
    fan_speed = ((fan_range - temp_diff) / fan_range) * 255;


    // Couleur de la LED Strip en Rouge 
    strip_r = 255;

  }else if(t < SB){ // Si temperature trop faible
    //Serial.println("Temperature too low, starting heater");

    rad_led_on = true;

    strip_b = 255;
  }else{
    //Serial.println("Temperature is ideal");
    strip_g = 255;
  }

  jdoc["status"]["fire"] = fire_detected;
  jdoc["status"]["heat"] = rad_led_on ? "ON" : "OFF";
  jdoc["status"]["cold"] = clim_led_on ? "ON" : "OFF";
  jdoc["status"]["fanspeed"] = fan_speed;

  serializeJson(jdoc, payload);
  Serial.println(payload);

  //StaticJsonDocument<256> jdoc_new;
  //deserializeJson(jdoc_new, payload);

  //String print_brightness = jdoc_new["brightness"];
  //String print_temperature = jdoc_new["temperature"];

  //Serial.println();
  //Serial.println("Reception and Deserialization of the String/Payload ");

  //Serial.print("\nBrightness: ");
  //Serial.print(print_brightness); // Valeur max du capteur - valeur actuelle du capteur -> valeur qui augmente avec le niveau de luminosité
  //Serial.print("\n");
  
  //Serial.print("Temperature: ");
  //Serial.print(print_temperature);
  //Serial.print(" C\n\n");


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