// ESParaSite_Rest.cpp

/* ESParasite Data Logger v0.6
        Authors: Andy (DocMadmag) Eakin

        Please see /ATTRIB for full credits and OSS License Info
        Please see /LIBRARIES for necessary libraries
        Please see /VERSION for Hstory

        All Derived Content is subject to the most restrictive licence of it's
   source.

        All Original content is free and unencumbered software released into the
   public domain.
*/

#include "ESParaSite_DebugUtils.h"
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <WiFiClient.h>


#include "ESParaSite.h"
#include "ESParaSite_HttpCore.h"

using namespace ESParaSite;

//+++ User Settings +++

// Enter the port on which you want the HTTP server to run (Default is 80).
// You will access the server at http://<ipaddress>:<port>*/
#define HTTP_REST_PORT 80

//*** DO NOT MODIFY ANYTHING BELOW THIS LINE ***

ESP8266WebServer http_server(HTTP_REST_PORT);

extern printchamber chamber_resource;
extern optics optics_resource;
extern ambient ambient_resource;
extern enclosure enclosure_resource;
extern status_data status_resource;
extern config_data config_resource;
extern rtc_eeprom_data rtc_eeprom_resource;

String getContentType(String filename);
bool handleFileRead(String path);
const char *htmlfile = "/index.html";

File fsUploadFile; // a File object to temporarily store the received file

void HttpCore::config_rest_server_routing() {
  http_server.on("/", handleRoot); // Call the 'handleRoot' function when a
                                   // client requests URI "/"
  http_server.onNotFound(
      handleNotFound); // When a client requests an unknown URI (i.e. something
                       // other than "/"), call function "handleNotFound"
  http_server.on("/printchamber", HTTP_GET, get_chamber);
  http_server.on("/optics", HTTP_GET, get_optics);
  http_server.on("/ambient", HTTP_GET, get_ambient);
  http_server.on("/enclosure", HTTP_GET, get_enclosure);
  http_server.on("/config", HTTP_GET, get_config);
  //http_server.on("/upload", HTTP_GET, do_upload_page);
  http_server.on("/upload", HTTP_POST, []() { 
        // if the client posts to the upload page
      
        http_server.send(200);
      }, // Send status 200 (OK) to tell the client we are ready to receive
      handleFileUpload // Receive and save the file
  );
  Serial.println("HTTP REST config complete");
}

void handleRoot() {
  http_server.send(
      200, "text/html",
      "<p>Please Browse to:</p><a "
      "href=\"/printchamber\">Printchamber</a></br><a "
      "href=\"/optics\">Optics</a></br><a href=\"/ambient\">Ambient</a></br><a "
      "href=\"/enclosure\">Enclosure</a></br><a href=\"/config\">Config</a>");
  // Send HTTP status 200 (Ok) and send some
  // text to the browser/client
}

void handleNotFound() {
  http_server.send(
      404, "text/plain",
      "404: Not found"); // Send HTTP status 404 (Not Found) when there's no
                         // handler for the URI in the request
}

void HttpCore::serve_http_client() { http_server.handleClient(); }

void HttpCore::start_http_server() { http_server.begin(); }
void HttpCore::stop_http_server() { http_server.stop(); }

/*
bool HttpCore::do_web_gui(
    String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  // If a folder is requested, send the index file
  if (path.endsWith("/"))
    path += "index.html";
  // Get the MIME type
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";

  // If the file exists, either as a compressed archive, or normal
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) {
    // If there's a compressed version available
    if (LittleFS.exists(pathWithGz))
      // Use the compressed version
      path += ".gz";
    // Open the file
    File file = LittleFS.open(path, "r");
    // Send it to the client
    http_server.streamFile(file, contentType);
    // Close the file again
    file.close();
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  // If the file doesn't exist, return false
  Serial.println(String("\tFile Not Found: ") + path);
  return false;
}
*/
void do_upload_page() {
  http_server.send(
      200, "text/html", "<form method=\"post\" enctype=\"multipart/form-data\">"
                        "<input type=\"file\" name=\"name\">"
                        "<input class=\"button\" type=\"submit\" value=\"Upload\">"
                        "</form>");
}

void get_chamber() {
  StaticJsonDocument<256> doc;

  doc["class"] = "chamber";
  doc["timestamp"] = status_resource.rtc_current_second;
  doc["chmb_temp_c"] = chamber_resource.dht_temp_c;
  doc["chmb_humidity"] = chamber_resource.dht_humidity;
  doc["chmb_dewpoint"] = chamber_resource.dht_dewpoint;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_optics() {
  StaticJsonDocument<256> doc;

  doc["class"] = "optics";
  doc["timestamp"] = status_resource.rtc_current_second;
  doc["uvindex"] = optics_resource.si_uvindex;
  doc["visible"] = optics_resource.si_visible;
  doc["infrared"] = optics_resource.si_infrared;
  doc["led_temp_c"] = optics_resource.mlx_amb_temp_c;
  doc["screen_temp_c"] = optics_resource.mlx_obj_temp_c;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_ambient() {
  StaticJsonDocument<256> doc;

  doc["class"] = "ambient";
  doc["timestamp"] = status_resource.rtc_current_second;
  doc["amb_temp_c"] = ambient_resource.bme_temp_c;
  doc["amb_humidity"] = ambient_resource.bme_humidity;
  doc["amb_pressure"] = ambient_resource.bme_barometer;
  doc["altitude"] = ambient_resource.bme_altitude;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_enclosure() {
  StaticJsonDocument<256> doc;

  doc["class"] = "enclosure";
  doc["timestamp"] = status_resource.rtc_current_second;
  doc["case_temp_c"] = enclosure_resource.case_temp;
  doc["lifetime_sec"] = enclosure_resource.life_sec;
  doc["screen_sec"] = enclosure_resource.lcd_sec;
  doc["led_sec"] = enclosure_resource.led_sec;
  doc["fep_sec"] = enclosure_resource.fep_sec;

  serializeJson(doc, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  http_server.send(200, "application/json", output);

  serializeJsonPretty(doc, Serial);
  Serial.println();
}

void get_config() {
  StaticJsonDocument<256> doc;
  StaticJsonDocument<256> doc2;

  doc["class"] = "eeprom";
  doc["timestamp"] = status_resource.rtc_current_second;
  doc["first_on_time64"] = rtc_eeprom_resource.first_on_timestamp;
  doc["last_write_time64"] = rtc_eeprom_resource.last_write_timestamp;
  doc["screen_life_sec"] = rtc_eeprom_resource.screen_life_seconds;
  doc["led_life_sec"] = rtc_eeprom_resource.led_life_seconds;
  doc["fep_life_sec"] = rtc_eeprom_resource.fep_life_seconds;

  serializeJson(doc, Serial);
  Serial.println();

  serializeJson(doc2, Serial);
  Serial.println();

  String output = "JSON = ";
  serializeJsonPretty(doc, output);
  serializeJsonPretty(doc2, output);
  http_server.send(200, "application/json", output);
}

String getContentType(String filename) {
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

void handleFileUpload() { // upload a new file to the LittleFS
  HTTPUpload &upload = http_server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    fsUploadFile =
        LittleFS.open(filename, "w"); // Open the file for writing in LittleFS
                                      // (create if it doesn't exist)
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(
          upload.buf,
          upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {     // If the file was successfully created
      fsUploadFile.close(); // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      http_server.sendHeader(
          "Location",
          "/success.html"); // Redirect the client to the success page
      http_server.send(303);
    } else {
      http_server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
