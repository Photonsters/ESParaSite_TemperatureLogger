// ESParaSite_ConfigFile.cpp

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
// BASED ON -
// Example: storing JSON configuration file in flash file system
//
// Uses ArduinoJson library by Benoit Blanchon.
// https://github.com/bblanchon/ArduinoJson
//
// Created Aug 10, 2015 by Ivan Grokhotkov.
//
// This example code is in the public domain.

#include <ArduinoJson.h>
#include <FS.h>

#include "ESParaSite.h"
#include "ESParaSite_FileCore.h"

extern ESParaSite::config_data config_resource;

bool ESParaSite::FileCore::loadConfig() {
  //  Serial.println(F("Inside loadconfig()"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {

  //  Serial.println(F("Inside if(!configFile)"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

    Serial.println(F("Failed to open config file."));
    return false;
  }

  //  Serial.println(F("About to check filesize"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  size_t size = configFile.size();

  //  Serial.println(F("Inside just checked config file size"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  if (size > 1024) {
    Serial.println(F("Config file size is too large."));
    return false;
  }

  // Allocate a buffer to store contents of the file.
  //  Serial.println(F("Allocate a buffer to store contents of the file"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  std::unique_ptr<char[]> buf(new char[size]);

  //  Serial.println(F("Aabout to byte read the file"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable.

  configFile.readBytes(buf.get(), size);

  //  Serial.println(F("about to serialize JSON"));
  //  Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  StaticJsonDocument<1024> doc;
  auto error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println(F("Failed to parse config file"));
    return false;
  }

  //  Serial.println(F("JSON deserialized about to place values in
  //  config_resource")); Serial.println(F("Waiting 3 Seconds..."));
  //  delay(3000);

  config_resource.cfg_wifi_ssid = doc["wifi_ssid"];
  //  Serial.println(F("wifi_ssid"));
  config_resource.cfg_wifi_password = doc["wifi_password"];
  //  Serial.println(F("wifi_password"));
  config_resource.cfg_pin_sda = doc["sda_pin"];
  //  Serial.println(F("sda_pin"));
  config_resource.cfg_pin_scl = doc["scl_pin"];
  //  Serial.println(F("scl_pin"));
  config_resource.cfg_mdns_enabled = doc["mdns_enabled"];
  //  Serial.println(F("mdns_enabled"));
  strncpy(config_resource.cfg_mdns_name, doc["mdns_name"], 32);
  int len = strlen(config_resource.cfg_mdns_name);
    if (len > 0 && config_resource.cfg_mdns_name[len - 1] == '\n') {
    config_resource.cfg_mdns_name[len - 1] = '\0';
  }
  //  Serial.println(F("mdns_name"));

  // This if statement currently crashes the system due to an interaction between strncmp and Null vlaues.
  // We need to clean this up when we fully implement backing up wifi config to config.json.
  /*
  if (!strncmp(config_resource.cfg_wifi_ssid, "", 32 )) {
    Serial.println("No Wifi config set in config.json");
    Serial.println("");
  } else {
    Serial.println("Wifi Config loaded from config.json");
    Serial.print("SSID: ");
    Serial.println(config_resource.cfg_wifi_ssid);
    Serial.print("PASSWORD: ");
    Serial.println(config_resource.cfg_wifi_password);
    Serial.println("");
  }
  */

  if (config_resource.cfg_mdns_enabled == 1) {
    Serial.println(F("mDNS enabled on URL:"));
    Serial.print(F("http://"));
    Serial.print(config_resource.cfg_mdns_name);
    Serial.println(F(".local"));
    Serial.println();
  }

  Serial.print(F("I2C Bus on Pins (SDA,SCL): "));
  Serial.print(config_resource.cfg_pin_sda);
  Serial.print(F(", "));
  Serial.println(config_resource.cfg_pin_scl);
  Serial.println();

  return true;
}

bool ESParaSite::FileCore::saveConfig() {
  StaticJsonDocument<200> doc;
  doc["wifi_ssid"] = config_resource.cfg_wifi_ssid;
  doc["wifi_password"] = config_resource.cfg_wifi_password;
  doc["sda_pin"] = config_resource.cfg_pin_sda;
  doc["scl_pin"] = config_resource.cfg_pin_scl;
  doc["mdns_enabled"] = config_resource.cfg_mdns_enabled;
  doc["mdns_name"] = config_resource.cfg_mdns_name;

  serializeJsonPretty(doc, Serial);
  Serial.println();

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println(F("Failed to open config file for writing"));
    return false;
  }

  serializeJson(doc, configFile);
  return true;
}