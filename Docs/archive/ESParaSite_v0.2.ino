/* ESParasite Data Logger v0.2
	Authors: Andy (DocMadmag) Eakin

	  Please see /ATTRIB for full credits and OSS License Info
  	Please see /LIBRARIES for necessary libraries
  	Please see /VERSION for Hstory

	All Derived Content is subject to the most restrictive licence of it's source.

	All Original content is free and unencumbered software released into the public domain.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SI1145.h>
#include <Adafruit_MLX90614.h>
#include <dht.h>
#include <EepromAT24C32.h>
#include <RtcDateTime.h>
#include <RtcDS3231.h>
#include <RtcTemperature.h>
#include <RtcUtility.h>
#include <BlueDot_BME280.h>

//+++ User Settings +++
//const char* wifi_ssid     = "yourwifinetwork";
//const char* wifi_password = "yourwifipassword";


//+++ Advanced Settings +++
// For precise altitude measurements please put in the current pressure corrected for the sea level
// Otherwise leave the standard pressure as default (1013.25 hPa);
// Also put in the current average temperature outside (yes, really outside!)
// For slightly less precise altitude measurements, just leave the standard temperature as default (15°C and 59°F);
#define SEALEVELPRESSURE_HPA (1013.25)
#define CURRENTAVGTEMP_C (15)
#define CURRENTAVGTEMP_F (59)

#define HTTP_REST_PORT 80

//Set the I2C address of your breakout board
//int bme_i2c_address = 0x77;
int bme_i2c_address = 0x76;


//Initialize Libraries
ESP8266WebServer http_rest_server(HTTP_REST_PORT);
dht12 DHT(0x5c);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Adafruit_SI1145 uv = Adafruit_SI1145();
BlueDot_BME280 bme;
RtcDS3231<TwoWire> Rtc(Wire);
EepromAt24c32<TwoWire> RtcEeprom(Wire);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
unsigned long delayTime;
int bmeDetected = 0;

struct printchamber {
  float dht_temp_c;
  float dht_humidity;
  float dht_dewpoint;
} chamberResource;

struct optics {
  float si_uvindex;
  float ledVisible;
  float ledInfrared;
  float ledTempC;
  float screenTempC;
} opticsResource;

struct ambient {
  float ambientTempC;
  float ambientHumidity;
  float ambientBarometer;
  float ambientAltitude;
} ambientResource;

struct enclosure {
  float caseTempC;
  float total_sec;
  float screen_sec;
  float ledLifeSec;
} enclosureResource;

RtcDateTime now;
char timestamp[14];

void loop(void) {
  http_rest_server.handleClient();
}

int init_wifi() {
  // Connect to WiFi network
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("\n\r \n\rConnecting to Wifi");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  return WiFi.status(); // return the WiFi connection status
}

void init_chamberResource() {
  chamberResource.dht_temp_c = 0;
  chamberResource.dht_humidity = 0;
  chamberResource.dht_dewpoint = 0;
}

void init_opticsResource() {
  opticsResource.si_uvindex = 0;
  opticsResource.ledVisible = 0;
  opticsResource.ledInfrared = 0;
  opticsResource.ledTempC = 0;
  opticsResource.screenTempC = 0;
}

void init_ambientResource() {
  ambientResource.ambientTempC = 0;
  ambientResource.ambientHumidity = 0;
  ambientResource.ambientBarometer = 0;
  ambientResource.ambientAltitude = 0;
}

void init_enclosureResource() {
  enclosureResource.caseTempC = 0;
  enclosureResource.total_sec = 0;
  enclosureResource.screen_sec = 0;
  enclosureResource.ledLifeSec = 0;
}

void config_rest_server_routing() {
  http_rest_server.on("/", HTTP_GET, []() {
    http_rest_server.send(200, "text/html",
                          "Welcome to the ESParasite REST Web Server");
  });
  http_rest_server.on("/printchamber", HTTP_GET, get_chamber);
  http_rest_server.on("/optics", HTTP_GET, get_optics);
  http_rest_server.on("/ambient", HTTP_GET, get_ambient);
  http_rest_server.on("/enclosure", HTTP_GET, get_enclosure);
  // http_rest_server.on("/enclosure", HTTP_POST, post_enclosure); //Not yet implemented
  // http_rest_server.on("/enclosure", HTTP_PUT, post_enclosure);  //Not yet implemented
}

void get_chamber () {

  readDhtSensor();
  readRtcData();
  create_timestamp(now);

  StaticJsonDocument<256> doc;

  doc["class"] = "chamber";
  doc["timestamp"] = timestamp;
  doc["seconds_t"] = chamberResource.dht_temp_c;
  doc["seconds_s"] = chamberResource.dht_humidity;
  doc["seconds_l"] = chamberResource.dht_dewpoint;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_rest_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_optics () {

  readSiSensor();
  readMlxSensor();
  readRtcData();
  create_timestamp(now);

  StaticJsonDocument<256> doc;

  doc["class"] = "optics";
  doc["timestamp"] = timestamp;
  doc["uvindex"] = opticsResource.si_uvindex;
  doc["visible"] = opticsResource.ledVisible;
  doc["infrared"] = opticsResource.ledInfrared;
  doc["led_temp_c"] = opticsResource.ledTempC;
  doc["screen_temp_c"] = opticsResource.screenTempC;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_rest_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_ambient() {

  readBmeSensor();
  readRtcData();
  create_timestamp(now);

  StaticJsonDocument<256> doc;

  doc["class"] = "ambient";
  doc["timestamp"] = timestamp;
  doc["amb_temp_c"] = ambientResource.ambientTempC;
  doc["amb_humidity"] = ambientResource.ambientHumidity;
  doc["amb_pressure"] = ambientResource.ambientBarometer;
  doc["altitude"] = ambientResource.ambientAltitude;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_rest_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_enclosure() {

  readRtcData();
  //  read_at24_data();   //Placeholder - Not yet Implemented
  create_timestamp(now);

  StaticJsonDocument<256> doc;

  doc["class"] = "enclosure";
  doc["timestamp"] = timestamp;
  doc["caseTempC"] = enclosureResource.caseTempC;
  doc["seconds_t"] = enclosureResource.total_sec;  //Placeholder - Not yet Implemented
  doc["seconds_s"] = enclosureResource.screen_sec; //Placeholder - Not yet Implemented
  doc["seconds_l"] = enclosureResource.ledLifeSec;    //Placeholder - Not yet Implemented

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_rest_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("");

  if (init_wifi() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(wifi_ssid);
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.print("Error connecting to: ");
    Serial.println(wifi_ssid);
  }

  init_chamberResource();
  init_opticsResource();
  init_ambientResource();
  init_enclosureResource();
  config_rest_server_routing();

  Serial.println("");
  Serial.println("ESParasite Data Logging Server");
  Serial.print("Compiled: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("");

  //start http rest server
  http_rest_server.begin();
  Serial.println("HTTP REST server started");
  Serial.println();

  // initialize I2C bus
  Serial.println("Initialize I2C bus");
  Wire.begin(0, 2);
  Serial.println("OK!");
  Serial.println();

  // initialize DHT12 sensor
  Serial.println("Initialize DHT12 Sensor");
  init_dht_sensor();
  Serial.println();

  // initialize SI1145 UV sensor
  Serial.println("Read SI1145 sensor");
  if (! uv.begin()) {
    Serial.println("SI1145 Initialization Failure");
  }
  Serial.println("OK!");
  Serial.println();

  // initialize MLX90614 temperature sensor
  Serial.println("Read MLX90614 sensor");
  if (! mlx.begin()) {
    Serial.println("MLX90614 Initialization Failure");
  }
  Serial.println("OK!");
  Serial.println();

  // initialize bme80 temperature sensor
  Serial.println("Read BME280 sensor");
  initBmeSensor();
  Serial.println();


  // initialize DS3231 RTC
  Serial.println("Read DS3231 RTC sensor");
  initRtcClock();
  Serial.println();

  //Dump all Sensor data to Serial
  Serial.println();
  Serial.println("Current Sensor Readings");
  Serial.println("============================================================");
  Serial.println();
  Serial.println("DS3231 Real-Time Clock Timestamp and Temperature:");
  readRtcData();
  Serial.println();

  Serial.println("DHT12 Print Chamber Environmental Data:");
  readDhtSensor();
  Serial.println();

  Serial.println("SI1145 UV and Light Sensor Data:");
  readSiSensor();
  Serial.println();

  Serial.println("MLX90614 Temp Sensor Data:");
  readMlxSensor();
  Serial.println();

  Serial.println("BME280 Temp Sensor Data:");
  readBmeSensor();
  Serial.println();
  Serial.println("ESParasite Ready!");
}

void  init_dht_sensor() {
  // initialize DHT12 temperature sensor
  unsigned long b = micros();
  dht::ReadStatus chk = DHT.read();
  unsigned long e = micros();

  Serial.print(F("Read DHT12 sensor: "));
  switch (chk)
  {
    case dht::OK:
      Serial.print(F("OK, took "));
      Serial.print (e - b); Serial.print(F(" usec, "));
      break;
    case dht::ERROR_CHECKSUM:
      Serial.println(F("Checksum error"));
      break;
    case dht::ERROR_TIMEOUT:
      Serial.println(F("Timeout error"));
      break;
    case dht::ERROR_CONNECT:
      Serial.println(F("Connect error"));
      break;
    case dht::ERROR_ACK_L:
      Serial.println(F("AckL error"));
      break;
    case dht::ERROR_ACK_H:
      Serial.println(F("AckH error"));
      break;
    default:
      Serial.println(F("Unknown error"));
      break;
  }
}

void initBmeSensor() {
  // initialize BME280 temperature sensor

  bme.parameter.communication = 0;
  bme.parameter.I2CAddress = bme_i2c_address;
  bme.parameter.sensorMode = 0b11;
  bme.parameter.IIRfilter = 0b100;
  bme.parameter.humidOversampling = 0b101;
  bme.parameter.tempOversampling = 0b101;
  bme.parameter.pressOversampling = 0b101;
  bme.parameter.pressureSeaLevel = SEALEVELPRESSURE_HPA;
  bme.parameter.tempOutsideCelsius = CURRENTAVGTEMP_C;
  bme.parameter.tempOutsideFahrenheit = CURRENTAVGTEMP_F;

  if (bme.init() != 0x60)
  {
    Serial.println(F("BME280 Sensor not found!"));
    bmeDetected = 0;
  }
  else
  {
    Serial.println(F("BME280 Sensor detected!"));
    bmeDetected = 1;
  }
  Serial.println();
}

void initRtcClock() {
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }

  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

}

void readRtcData () {
  Serial.println("===================");
  if (!Rtc.IsDateTimeValid())
  {
    if (Rtc.LastError() != 0)
    {
      Serial.print("RTC communications error = ");
      Serial.println(Rtc.LastError());
    }
    else
    {
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();

  RtcTemperature temp = Rtc.GetTemperature();
  enclosureResource.caseTempC = (temp.AsFloatDegC());
  Serial.print(enclosureResource.caseTempC);
  Serial.println("°C");

  delay(500);
}

void readDhtSensor() {
  Serial.println("===================");

  //First DHT measurement is stale, so we measure, wait ~2 seconds, then measure again.
  DHT.getTemperature();
  DHT.getHumidity();
  DHT.dewPoint();

  delay(2500);

  Serial.print(F("Temperature (°C): "));
  chamberResource.dht_temp_c = ((float)DHT.getTemperature() / (float)10);
  Serial.println(((int)chamberResource.dht_temp_c));

  Serial.print(F("Humidity: "));
  chamberResource.dht_humidity = ((float)DHT.getHumidity() / (float)10);
  Serial.print(((int)chamberResource.dht_humidity));
  Serial.println("%");

  Serial.print(F("Dew Point (°C): "));
  chamberResource.dht_dewpoint = ((float)DHT.dewPoint());
  Serial.println(((int)chamberResource.dht_dewpoint));

  delay(1000);
}

void readSiSensor() {
  Serial.println("===================");

  opticsResource.si_uvindex = uv.readUV();
  opticsResource.si_uvindex /= 100.0;
  Serial.print("UV Index: ");
  Serial.println((int)opticsResource.si_uvindex);

  opticsResource.ledVisible = uv.readVisible();
  Serial.print("Vis: ");
  Serial.println(opticsResource.ledVisible);

  opticsResource.ledInfrared = uv.readIR();
  Serial.print("IR: ");
  Serial.println(opticsResource.ledInfrared);

  delay(1000);
}

void readMlxSensor() {
  Serial.println("===================");

  opticsResource.ledTempC = mlx.readAmbientTempC();
  Serial.print("Ambient = ");
  Serial.print(opticsResource.ledTempC);
  Serial.print("°C\t");
  Serial.print(mlx.readAmbientTempF());
  Serial.println("°F");

  opticsResource.screenTempC = mlx.readObjectTempC();
  Serial.print("Object = ");
  Serial.print(opticsResource.screenTempC);
  Serial.print("°C\t");
  Serial.print(mlx.readObjectTempF());
  Serial.println("°F");
  Serial.println();

  delay(1000);
}

void readBmeSensor() {
  Serial.println("===================");
  //  if (bmeDetected)
  //  {
  ambientResource.ambientTempC = bme.readTempC();
  Serial.print(F("Temperature Sensor:\t\t"));
  Serial.print(ambientResource.ambientTempC);
  Serial.print("°C\t");
  Serial.print(bme.readTempF());
  Serial.println("°F");

  ambientResource.ambientHumidity = bme.readHumidity();
  Serial.print(F("Humidity Sensor:\t\t"));
  Serial.print(ambientResource.ambientHumidity);
  Serial.println("%");

  ambientResource.ambientBarometer = bme.readPressure();
  Serial.print(F("Pressure Sensor [hPa]:\t"));
  Serial.print(ambientResource.ambientBarometer);
  Serial.println(" hPa");

  ambientResource.ambientAltitude = bme.readAltitudeMeter();
  Serial.print(F("Altitude Sensor:\t\t"));
  Serial.print(ambientResource.ambientAltitude);
  Serial.print("m\t");
  Serial.print(bme.readAltitudeFeet());
  Serial.println("ft");
  //  }

  /*  else
    {
      Serial.print(F("Temperature Sensor [°C]:\t\t"));
      Serial.println(F("Null"));
      Serial.print(F("Temperature Sensor [°F]:\t\t"));
      Serial.println(F("Null"));
      Serial.print(F("Humidity Sensor [%]:\t\t\t"));
      Serial.println(F("Null"));
      Serial.print(F("Pressure Sensor [hPa]:\t\t"));
      Serial.println(F("Null"));
      Serial.print(F("Altitude Sensor [m]:\t\t\t"));
      Serial.println(F("Null"));
      Serial.print(F("Altitude Sensor [ft]:\t\t\t"));
      Serial.println(F("Null"));
    }
  */
  Serial.println();
  Serial.println();

  delay(1000);

}

int convertCtoF(int temp_c)  {
  int temp_f;
  temp_f = ((int)round(1.8 * temp_c + 32));
  return temp_f;
}

#define countof(a) (sizeof(a) / sizeof(a[0]))

void printDateTime(const RtcDateTime & dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  Serial.print(datestring);
}

void create_timestamp(const RtcDateTime & dt)
{
  snprintf_P(timestamp,
             countof(timestamp),
             PSTR("%04u%02u%02u%02u%02u%02u"),
             dt.Year(),
             dt.Month(),
             dt.Day(),
             dt.Hour(),
             dt.Minute(),
             dt.Second() );
  Serial.print(timestamp);
}
