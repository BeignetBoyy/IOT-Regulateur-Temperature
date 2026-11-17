/*--------------------------------*/
void setup_led(int LEDpin, int mode, int status){
  pinMode(LEDpin, mode);
  digitalWrite(LEDpin, status);// Set outputs to LOW
}

/*--------------------------------*/
float get_temperature(DallasTemperature tempSensor) {
  tempSensor.requestTemperaturesByIndex(0);
  float t = tempSensor.getTempCByIndex(0);
  return t;
}
/*--------------------------------*/
int get_light(int LightPin) {
  int sensorValue = analogRead(LightPin);
  return sensorValue;
}

