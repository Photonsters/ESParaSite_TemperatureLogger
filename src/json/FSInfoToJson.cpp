// ConfigFile.cpp

/* ESParasite Data Logger
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
#include <LittleFS.h>

#include "ESParaSite.h"
#include "DebugUtils.h"
#include "FileCore.h"
#include "Http.h"
#include "Json.h"

using namespace ESParaSite;

extern configData configResource;

void FileCore::getFSInfo(int mode) {

  FSInfo fs_info;
  LittleFS.info(fs_info);
  if (mode == 1) {
    Serial.print("Total Filesystem Bytes:\t");
    Serial.println(fs_info.totalBytes);
    Serial.print("Used Filesystem Bytes:\t");
    Serial.println(fs_info.usedBytes);
  } else if (mode == 2) {
    StaticJsonDocument<200> doc;
    doc["tfsb"] = fs_info.totalBytes;
    doc["ufsb"] = fs_info.usedBytes;
    HttpHandleJson::serializeSendJson(doc);
    return;
  }

  File root = LittleFS.open("/", "r");
  File file = root.openNextFile();
  if (mode == 1) {
    while (file) {
      Serial.print(file.name());
      Serial.print("\t\t");
      Serial.println(file.size());
      file = root.openNextFile();
    }
  } else if (mode == 3) {
    // We need to find a better way to do this since we can only fit ~50
    // files in this JSON Doc. look into splitting this and doing HTTP Chunks.
    // https://gist.github.com/spacehuhn/6c89594ad0edbdb0aad60541b72b2388
    int iter = 0;

    ESParaSite::HttpHandleJson::sendContentLengthUnknown();

    while (file)
    {
      DynamicJsonDocument parentDoc(2048);
      DynamicJsonDocument nestedDoc(80);
      for (int i = 0; i < 23; i++) {

        JsonObject nested = nestedDoc.to<JsonObject>();

        String fName = file.name();
        nested["fName"] = fName;
        nested["fSize"] = file.size();

        String child;
        serializeJson(nestedDoc, child);
        parentDoc.add(serialized(child));
        
        file = root.openNextFile();
        if (!file) {
          break;
        }
      }
      if (iter == 0){
        ESParaSite::HttpHandleJson::serializeSendJson(parentDoc);
        iter++;
      } else {
        ESParaSite::HttpHandleJson::serializeSendJsonPartN(parentDoc);
        iter++;
      }
    }
    
  }
}