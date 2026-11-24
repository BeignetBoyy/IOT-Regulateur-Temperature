#include "ESPAsyncWebServer.h"
#include "routes.h"
#include "FS.h"
#include <LittleFS.h>

#define USE_SERIAL Serial
extern char payload[512];
extern StaticJsonDocument<512> json;
extern float SH_VENTILO, LUM_FEU, TEMP_FEU;

/*===================================================*/
String processor(const String & var){

  if (var == "TEMPERATURE") {
    return json["status"]["temperature"];
  }else if (var == "LIGHT") { 
    return json["status"]["light"];
  }else if (var == "LT") {
    return json["regul"]["lt"];
  }else if (var == "HT") {
    return json["regul"]["ht"];
  }else if (var == "FAN_MAX") {
    return String(SH_VENTILO);
  }else if (var == "TEMP_FEU") {
    return String(TEMP_FEU);
  }else if (var == "LUM_FEU") {
    return String(LUM_FEU);
  }else if (var == "COOLER") {
    return json["status"]["cold"];
  }else if (var == "HEATER") {
    return json["status"]["heat"];
  }else if (var == "FAN_SPEED") {
    return json["status"]["fanspeed"];
  }else if (var == "FIRE") {
    return json["status"]["fire"];
  }else if (var == "SSID") {
    return json["net"]["ssid"];
  }else if (var == "MAC") {
    return json["net"]["mac"];
  }else if (var == "IP") {
    return json["net"]["ip"];
  }else {
    return String(); // parce que !=> cf doc de asyncwebserver
  }
}

/*===================================================*/
void setup_http_routes(AsyncWebServer* server) {
  server->serveStatic("/", LittleFS, "/").setTemplateProcessor(processor);  

  auto root_handler = server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      USE_SERIAL.printf("Root route requested !\n"); 
      request->send(LittleFS, "/index.html", String(), false, processor);   
    });
  
  #ifdef CSS
  // Route to load style.css file
  server.on("/esp.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/esp.css", "text/css");
  });
  #endif
  
  server->on("/json", HTTP_GET, [](AsyncWebServerRequest *request){
    USE_SERIAL.printf("GET /json request \n");
    request->send(200, "application/json", payload);
  });

  server->on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    String temp = json["status"]["temperature"];
    request->send(200, "text/plain", temp);
  });

  server->on("/light", HTTP_GET, [](AsyncWebServerRequest *request){
    String light = json["status"]["light"];
    request->send(200, "text/plain", light);
  });


  server->on("/value", HTTP_GET, [](AsyncWebServerRequest *request){
    USE_SERIAL.printf("GET /value request \n"); 
    StaticJsonDocument<256> send_json;

    if (request->hasArg("light")){
      send_json["light"] = json["status"]["light"];
    }
    if (request->hasArg("temperature")){
      send_json["temperature"] = json["status"]["temperature"];
    }
    if (request->hasArg("fire")){
      send_json["fire"] = json["status"]["fire"];
    }
    if (request->hasArg("lt")){
      send_json["lt"] = json["regul"]["lt"];
    }
    if (request->hasArg("ht")){
      send_json["ht"] = json["regul"]["ht"];
    }

    if (send_json.isNull()){
      send_json["temperature"] = json["status"]["temperature"];
      send_json["light"] = json["status"]["light"];
      send_json["fire"] = json["status"]["fire"];
      send_json["lt"] = json["regul"]["lt"];
      send_json["ht"] = json["regul"]["ht"];
    }

    String response;
    serializeJson(send_json, response);

    request->send(200, "application/json", response);
  });

  server->on("/set", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasArg("light_threshold")) { // request may have arguments
      LUM_FEU = atoi(request->arg("light_threshold").c_str());
      request->send_P(200, "text/plain", "Threshold Set !");
    }
  });

  server->on("/target", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("Receive Request for a periodic report !"); 
    if (request->hasArg("ip") &&
      request->hasArg("port") &&
      request->hasArg("sp")) {
        target_ip = request->arg("ip");
        target_port = atoi(request->arg("port").c_str());
        target_sp = atoi(request->arg("sp").c_str());
      }
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  // If request doesn't match any route, returns 404.
  server->onNotFound([](AsyncWebServerRequest *request){
      request->send(404);
    });
}
/*===================================================*/
