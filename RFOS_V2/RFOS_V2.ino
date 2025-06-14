// Proudly NOT Made in China
// ... But the compiler tho 💀

#include <WiFi.h>
#include <SD.h>
#include <SPI.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESPmDNS.h>
#include <ElegantOTA.h> //Replaced from PrettyOTA.h (due to legal lawsuits/DMCA to LostInCompilation's PrettyOTA)
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <vector>
#include <map>
#include "Settings.h"
// DS1302 pins configs
#define DS1302_CE_PIN 12
#define DS1302_IO_PIN 13
#define DS1302_CLK_PIN 14

//LEDs
#define RED 27
#define GREEN 26
#define BLUE 25


// IDK if this still runs on the DS1302 that is fucked even if its new, idc... its just there
ThreeWire rtcWire(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);
RtcDS1302<ThreeWire> rtc(rtcWire);

// Chat feat. WebSocket
AsyncWebSocket ws("/ws");

// Chat STL Containers
std::map<uint32_t, String> wsUsernames;
AsyncWebServer server(80);
AsyncEventSource event("/events");


// ALWAYS TRUE, NEVER FALSE
#define USE_STATION true
;

// IP Configurations
IPAddress local_IP(192, 168, 0, 3);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);

// MFRC Instances and Pins
#define RST_PIN 22
#define SS_PIN 21
#define RFID_DATA_BLOCK 4
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Containers of Shit
File uploadFile;
String newName = "";

unsigned long ota_progress_millis = 0;

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  
}


// Utility: return MIME type based on file extension
String getContentType(const String &filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".xlsx")) return "application/vnd.ms-excel";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/pdf";
  else if (filename.endsWith(".docx"))
    return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
  else if (filename.endsWith(".pptx"))
    return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
  return "text/plain";
}

// ====== WebSocket Event Handler for Chat ======
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
               void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client connected: %u\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client disconnected: %u\n", client->id());

    wsUsernames.erase(client->id());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT) {
      String msg = "";
      for (size_t i = 0; i < len; i++) {
        msg += (char)data[i];
      }
      // If the client isn't registered yet, expect a registration message
      if (wsUsernames.find(client->id()) == wsUsernames.end()) {
        if (msg.startsWith("register:")) {
          String desiredName = msg.substring(9);
          desiredName.trim();
          bool taken = false;
          for (auto &entry : wsUsernames) {
            if (entry.second == desiredName) {
              taken = true;
              break;
            }
          }
          if (taken || desiredName == "") {
            client->text("error: Name already in use or invalid. Try another one.");
          } else {
            wsUsernames[client->id()] = desiredName;
            client->text("success: Registered as " + desiredName);
            Serial.printf("Client %u registered as %s\n", client->id(), desiredName.c_str());
          }
        } else {
          client->text("error: Please register first with 'register:YourName'");
        }
      } else {
        // Already registered, treat the incoming message as a chat message.
        String username = wsUsernames[client->id()];
        String chatMessage = username + ": " + msg;
        // Broadcast the chat message to all connected WebSocket clients.
        server->textAll(chatMessage);
        Serial.println("Chat message: " + chatMessage);
      }
    }
  }
}

void deleteFolderRecursive(const char *folderPath) {
  File dir = SD.open(folderPath);
  if (!dir || !dir.isDirectory()) {
    Serial.println("Folder not found or not a directory.");
    return;
  }

  File entry;
  while ((entry = dir.openNextFile())) {
    String entryPath = String(folderPath) + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      deleteFolderRecursive(entryPath.c_str());  // recursive call
    } else {
      entry.close();
      SD.remove(entryPath.c_str());  // delete file
    }
  }
  dir.close();
  SD.rmdir(folderPath);  // delete the empty folder
}



// Asynchronous file upload handler
void handleUpload(AsyncWebServerRequest *request, const String &filename,
                  size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("Upload Start: %s\n", filename.c_str());
    // Ensure filename starts with '/'
    String path = "/uploads/" + filename;
    uploadFile = SD.open(path, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Failed to open file for writing");
      return;
    }
  }
  if (uploadFile) {
    if (len > 0)
      uploadFile.write(data, len);
    if (final) {
      uploadFile.close();
      Serial.printf("Upload Success: %s\n", filename.c_str());
    }
  }
}

// Asynchronous self overwrite
void overwriteHTML(AsyncWebServerRequest *request, const String &filename,
                   size_t index, uint8_t *data, size_t len, bool final) {
  if (index == 0) {
    Serial.printf("Upload Start: %s\n", filename.c_str());
    // Ensure filename starts with '/'
    String path = "/src/" + filename;
    SD.remove(path);
    uploadFile = SD.open(path, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Failed to open file for writing");
      return;
    }
  }
  if (uploadFile) {
    if (len > 0)
      uploadFile.write(data, len);
    if (final) {
      uploadFile.close();
      Serial.printf("Upload Success: %s\n", filename.c_str());
    }
  }
}


String generateFileList() {
  String output = "{\"files\":[";
  bool first = true;

  File root = SD.open("/uploads");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        // strip “/uploads/” prefix
        String name = file.name();
        if (name.startsWith("/uploads/")) {
          name = name.substring(strlen("/uploads/"));
        }
        // comma-separate
        if (!first) {
          output += ",";
        }
        first = false;
        // quote the filename
        output += "\"" + name + "\"";
      }
      file = root.openNextFile();
      yield();
    }
    root.close();
  }

  output += "]}";
  return output;
}

void setupServer() {
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
  server.addHandler(&event);
  server.serveStatic("/", SD, "/src/").setDefaultFile("index.html").setCacheControl("max-age=600");
  server.serveStatic("/wallpaperid", SD, "/src/wallpaper.png").setCacheControl("max-age=600");
  server.serveStatic("/favicon.png", SD, "/src/icon.png").setCacheControl("max-age=600");
  server.serveStatic("/cur.png", SD, "/src/cursor.png").setCacheControl("max-age=600");
  server.on(
    "/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "Upload received");
    },
    handleUpload);

  server.on("/futuristic", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SD, "/src/futuristic.html", "text/html");
  });


  server.on("/nuke_server", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    deleteFolderRecursive("/uploads");
    request->send(200, "text/plain", "Nuked the folder.");
  });


  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = generateFileList();
    request->send(200, "application/json", json);
    yield();
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Rebooting Server");
    ESP.restart();
  });

  server.on("/warn_update", HTTP_GET, [](AsyncWebServerRequest *request) {
    event.send("Developer pushed an update and the server needs reboot, please notify the developer if important tasks are being done within an hour", "message", millis());
    request->send(200, "text/plain", "Notified Clients About the Update");
  });

  server.on("/test-toast", HTTP_GET, [](AsyncWebServerRequest *request) {
    event.send("Koharu Hello There! I'm NOT a toast :(", "toast", millis());
    request->send(200, "text/plain", "Toast served");
  });

  server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("file")) {
      request->send(400, "text/plain", "Bad Request: file parameter missing");
      return;
    }
    String filename = request->getParam("file")->value();
    String path = "/uploads/" + filename;
    if (!SD.exists(path)) {
      request->send(404, "text/plain", "File not found");
      return;
    }
    AsyncWebServerResponse *response = request->beginResponse(SD, path, getContentType(path));
    response->addHeader("Content-Disposition", "attachment; filename=" + filename);
    request->send(response);
  });
  server.on("/delete", HTTP_DELETE, [](AsyncWebServerRequest *request) {
          if (!request->hasParam("file")) {
            request->send(400, "text/plain", "Bad Request: file parameter missing");
            return;
          }
          String filename = request->getParam("file")->value();
          String path = "/uploads/" + filename;

          if (!SD.exists(path)) {
            request->send(404, "text/plain", "File not found");
            return;
          }

          if (SD.remove(path)) {
            request->send(200, "text/plain", "File deleted successfully");
          } else {
            request->send(500, "text/plain", "Error deleting file");
          }
        })
    .setAuthentication("Kennard I.", "ArduiNovice_Kennard");
  Serial.println("HTTP server started");
  yield();
}

void fileServerTask(void *parameter) {
  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  SPI.begin();
#ifdef PRODUCTION_READY
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  mfrc522.PCD_Init();
#endif
  WiFi.begin(UPLINK_SSID, UPLINK_PASS);

  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
    digitalWrite(BLUE, HIGH);
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
    return;
  }
  Serial.println("SD Card mounted successfully.");

  if (!SD.exists("/uploads")) {
    SD.mkdir("/uploads");
    Serial.println("Created /uploads folder.");
  }
  delay(1000);
  if (!SD.exists("/attendance")) {
    SD.mkdir("/attendance");
    Serial.println("Created /attendance folder.");
  }
  delay(500);
  if (!SD.exists("/attendance/attendance.csv")) {
    File file = SD.open("/attendance/attendance.csv", FILE_WRITE);
    if (file) {
      file.close();
    }
  }



  Serial.println("MFRC522 initialized.");
  rtc.Begin();
  if (!rtc.IsDateTimeValid()) {
    rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));
    Serial.println("RTC was not valid, so it was set to compile time.");
  } else {
    Serial.println("RTC is valid.");
  }

  MDNS.begin(DNS_ADDRESS);

  ElegantOTA.begin(&server);  // Start ElegantOTA
  // ElegantOTA callbacks
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  ElegantOTA.setAutoReboot(true);
  setupServer();
  server.begin();

  xTaskCreatePinnedToCore(
    fileServerTask,
    "FileServerTask",
    4096,
    NULL,
    1,
    NULL,
    1);
}

void loop() {
  ElegantOTA.loop();
#ifdef ENABLE_MFRC522
  digitalWrite(RED, LOW);
  digitalWrite(BLUE, HIGH);
  digitalWrite(GREEN, LOW);

  if (Serial.available() > 0) {
    newName = Serial.readStringUntil('\n');
    newName.trim();
    if (newName.length() > 0) {
      Serial.print("New name set via Serial: ");
      Serial.println(newName);
    }
  }

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {

    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    byte block = RFID_DATA_BLOCK;
    byte buffer[18];
    byte size = sizeof(buffer);

    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Authentication failed: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(1000);
      return;
    }

    status = mfrc522.MIFARE_Read(block, buffer, &size);
    String oldName = "";
    if (status == MFRC522::STATUS_OK) {
      char nameBuffer[17];
      memcpy(nameBuffer, buffer, 16);
      nameBuffer[16] = '\0';
      oldName = String(nameBuffer);
      oldName.trim();
    } else {
      Serial.print("Reading failed: ");
      Serial.println(mfrc522.GetStatusCodeName(status));
    }

    if (newName.length() > 0) {

      byte dataBlock[16];
      memset(dataBlock, 0, 16);
      int len = newName.length();
      if (len > 16) len = 16;
      newName.toCharArray((char *)dataBlock, len + 1);

      status = mfrc522.MIFARE_Write(block, dataBlock, 16);
      if (status == MFRC522::STATUS_OK) {
        Serial.print("Card update: Previous Owner: ");
        Serial.print(oldName);
        Serial.print(" | New Owner: ");
        Serial.println(newName);
      } else {
        Serial.print("Writing failed: ");
        Serial.println(mfrc522.GetStatusCodeName(status));
      }
      newName = "";
    } else {

      File csvFile = SD.open("/attendance/attendance.csv", FILE_APPEND);
      if (csvFile) {
        RtcDateTime now = rtc.GetDateTime();
        char dateBuffer[30];
        sprintf(dateBuffer, "%04d-%02d-%02d %02d:%02d:%02d", now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
        csvFile.print(oldName);
        csvFile.print(",");
        csvFile.println(dateBuffer);
        csvFile.close();
        Serial.print("Attendance logged for: ");
        Serial.print(oldName);
        Serial.print(" at ");
        Serial.println(dateBuffer);
        digitalWrite(GREEN, HIGH);
        digitalWrite(RED, LOW);
        digitalWrite(BLUE, LOW);
      } else {
        Serial.println("Failed to open attendance CSV file.");
      }
    }


    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(1000);  // Delay to prevent multiple reads in rapid succession.
  }
#endif

  // Nothing else needed in loop; the web server and WebSocket run asynchronously.
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
