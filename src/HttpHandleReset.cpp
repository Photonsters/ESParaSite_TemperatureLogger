// HttpHandler.cpp

/* ESParasite Data Logger v0.9
        Authors: Andy (DocMadmag) Eakin

        Please see /ATTRIB for full credits and OSS License Info
        Please see /LIBRARIES for necessary libraries
        Please see /VERSION for Hstory

        All Derived Content is subject to the most restrictive licence of it's
        source.

        All Original content is free and unencumbered software released into the
        public domain.

        The Author(s) are extremely grateful for the amazing open source
        communities that work to support all of the sensors, microcontrollers,
        web standards, etc.
*/

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <LittleFS.h>
#include <WiFiClient.h>

#include "ESParaSite.h"
#include "SensorsCore.h"
#include "Http.h"


extern ESP8266WebServer server;

extern ESParaSite::rtcEepromData rtcEepromResource;

void ESParaSite::HttpHandler::handleResetScreen() {
  server.send(200, "text/html", "Success!");
  Serial.print(F("Resetting LCD Screen Counter"));
  rtcEepromResource.eepromScreenLifeSec = 0;
}

void ESParaSite::HttpHandler::handleResetFep() {
  server.send(200, "text/html", "Success!");
  Serial.print(F("Resetting FEP Counter"));
  rtcEepromResource.eepromVatLifeSec = 0;
}

void ESParaSite::HttpHandler::handleResetLed() {
  server.send(200, "text/html", "Success!");
  Serial.print(F("Resetting LED Counter"));
  rtcEepromResource.eepromLedLifeSec = 0;
}

void ESParaSite::HttpHandler::handleSetClock(){
  String message = "";
  if (server.arg("TimeStamp")==""){
    message = "Time Argument not found";
  } else {
    Serial.print("Old Timestamp\t");
    Serial.println(ESParaSite::Sensors::readRtcEpoch());
    message = "Setting RTC clock";
    String tString = server.arg("TimeStamp");
    Serial.print("Set Timestamp\t");
    Serial.println(tString);
    char tChar[(tString.length()) + 1];
    strncpy(tChar,tString.c_str(),tString.length());
    ESParaSite::Sensors::setRtcfromEpoch(atoll(tChar));
    Serial.print("New Timestamp\t");
    Serial.println(ESParaSite::Sensors::readRtcEpoch());
  }

  server.send(200, "text/plain", message);
}

void ESParaSite::HttpHandler::getResetScreen() {
  server.send(200, "text/html",
              "<font size=\"+3\">WARNING - This will reset the lifetime"
              "counter of your LCD Screen to 0!!!</font><br>"
              "<form method=\"post\">"
              "<input type=\"submit\" name=\"name\" "
              "value=\"Reset\">"
              "</form>"
              "Please do not immediately turn off your printer. This change"
              "may take up to 30 seconds to be saved.<br>");
}

void ESParaSite::HttpHandler::getResetFep() {
  server.send(200, "text/html",
              "<font size=\"+3\">WARNING - This will reset the lifetime"
              "counter of your Vat FEP to 0!!!</font><br>"
              "<form method=\"post\">"
              "<input type=\"submit\" name=\"name\" "
              "value=\"Reset\">"
              "</form>"
              "Please do not immediately turn off your printer. This change"
              "may take up to 30 seconds to be saved.<br>");
}

void ESParaSite::HttpHandler::getResetLed() {
  server.send(200, "text/html",
              "<font size=\"+3\">WARNING - This will reset the lifetime"
              "counter of your LED Array to 0!!!</font><br>"
              "<form method=\"post\">"
              "<input type=\"submit\" name=\"name\" "
              "value=\"Reset\">"
              "</form>"
              "Please do not immediately turn off your printer. This change"
              "may take up to 30 seconds to be saved.<br>");
}
