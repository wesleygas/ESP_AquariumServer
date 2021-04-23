#include "ArduinoJson.h"
#include "AsyncJson.h"
#include "FS.h"
#include "helpers.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>

NTPtime NTPch(NTP_SERVER);

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define color_number 2

#define MANUAL_TIMEOUT 5000

int color_pins[color_number] = {D5, D6};

char color_names[color_number][3] = {"ww", "cw"};

LightData color_params[color_number];
int manualBrightness[color_number];

const char *ssid = WIFI_SSID;
const char *password = WIFI_AUTH;

RtcDS3231<TwoWire> Rtc(Wire);

AsyncWebServer server(80);

static std::vector<AsyncClient*> tcpClients; // a list to hold all clients

char tempoStr[9];

union {
  byte buf[8];
  struct 
  {
    unsigned int ww;
    unsigned int cw;
  } value;
} lightPack;

bool updateRTC = false;
bool alarm = false;
bool isManual = false;

unsigned long manualExpiresAt;

int last_hour = 0;

void loadJsonConfig(JsonObject &root, bool save) {
  for (int i = 0; i < color_number; i++) {
    JsonObject &color = root[color_names[i]];
    fillLightData(&color_params[i], color["start"], color["end"], color["max"], color["limit"]);
  }
  if (save) {
    File configFile = SPIFFS.open("/lightData.json", "w");
    if (root.printTo(configFile) == 0) {
      //Serial.println(F("Failed to write to file"));
    }
  }
}


void loadInternalConfigs() {
  StaticJsonBuffer<350> jsonBuffer;
  File configFile = SPIFFS.open("/lightData.json", "r");
  JsonObject &root = jsonBuffer.parseObject(configFile);
  configFile.close();
  if (!root.success()) {
    //Serial.println("Failed to open/parse object");
    while (1)
      yield(); //HALT
  } else {
    loadJsonConfig(root, false);
  }
}

void setupStaticFiles() {
  //Serve home
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/chart.html", String());
  });

  //Styling files
  server.on("/chart.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/chart.css", "text/css");
  });

  server.on("/materialize.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/materialize.min.css", "text/css");
  });

  server.on("/nouislider.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/nouislider.css", "text/css");
  });

  server.on("/chart.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/chart.js", String());
  });

  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/highcharts.js", String());
  });

  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/jquery.min.js", String());
  });

  server.on("/materialize.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/materialize.min.js", String());
  });

  server.on("/nouislider.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/nouislider.js", String());
  });

  server.on("/initialize.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/initialize.js", String());
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/favicon.ico", "image/png");
  });
}

void setupOTA() {
  ArduinoOTA.onStart([]() {
    //Serial.println("Start");
  });
  ArduinoOTA.setPassword(OTA_AUTH);
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  /*
  ArduinoOTA.onError([](ota_error_t error) {
    //Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR);
      //Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR);
      //Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR);
      //Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR);
      //Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR);
      //Serial.println("End Failed");
  }); */
  ArduinoOTA.begin();
}

void setupWifi() {
  WiFi.begin(ssid, password);
  //Serial.print("Connecting to WiFi");
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED and retries < 10) {
    delay(500);
    Serial.write('.');
    retries++;
  }
  //Serial.println(WiFi.localIP());
}

void setManualLightValues(){
  if(lightPack.value.cw < 1024 && lightPack.value.ww < 1024){
    manualBrightness[1] = lightPack.value.cw;
    manualBrightness[0] = lightPack.value.ww;
    isManual = true;
    manualExpiresAt = millis() + MANUAL_TIMEOUT;
  }
  for(int i = 0; i < color_number; i++){
    analogWrite(color_pins[i], manualBrightness[i]);
  }
}

static void handleData(void* arg, AsyncClient* client, void *data, size_t len) {
	////Serial.println("\nBuff: ");
  for(unsigned int i = 0u; (i < len && i < 8u); i++){
    lightPack.buf[i] = ((uint8_t*)data)[i];
    //Serial.printf("0x%X ",lightPack.buf[i]);
  }
  setManualLightValues();
  //Serial.printf("\n data received from client %s: cw: %d ww: %d\n", client->remoteIP().toString().c_str(), lightPack.value.cw, lightPack.value.ww);

	// reply to client
	if (client->space() > 32 && client->canSend()) {
		client->add("OK",3);
		client->send();
	}
}

static void handleNewClient(void* arg, AsyncClient* client) {
	//Serial.printf("\n new client has been connected to server, ip: %s", client->remoteIP().toString().c_str());

	// add to list
	tcpClients.push_back(client);
	
	// register events
	client->onData(&handleData, NULL);
	//client->onError(&handleError, NULL);
	//client->onDisconnect(&handleDisconnect, NULL);
	//client->onTimeout(&handleTimeOut, NULL);
}

void setupTCPServer(){
  //Serial.println("Starting TCP Server");
  AsyncServer* tcpServer = new AsyncServer(7050); // start listening on tcp port 7050
  tcpServer->onClient(&handleNewClient, tcpServer);
  tcpServer->begin();
}

void setupPins() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  for (int i = 0; i < color_number; i++) {
    pinMode(color_pins[i], OUTPUT);
  }
}

bool setupRTC(bool forceUpdate = false) {
  bool success = true;
  if (!Rtc.IsDateTimeValid() || forceUpdate) {
    if (Rtc.LastError() != 0) {
      //Serial.printf("RTC communication error = %d\n", Rtc.LastError());
      success = false;
    } else {
      //Find a way to get time from ntp
      strDateTime dateTime = NTPch.getNTPtime(-3.0, 0);
      if (dateTime.valid) {
        //Serial.println("Updating via NTP");
        RtcDateTime compiled = RtcDateTime(
            dateTime.year,
            dateTime.month,
            dateTime.day,
            dateTime.hour,
            dateTime.minute,
            dateTime.second);
        Rtc.SetDateTime(compiled);
        //Serial.println("Done");
      } else {
        //Serial.println("Failed to get NTP");
        success = false;
      }
    }
  }
  if (!Rtc.GetIsRunning()) {
    //Serial.printf("RTC wasnt running");
    Rtc.SetIsRunning(true);
  }
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
  return success;
}

void timeCheck(RtcDateTime nowr) {
  alarm = Rtc.LastError() != 0;
  if (updateRTC) {
    updateRTC = false;
    int i = 3;
    while (!setupRTC(true) && i--)
      delay(500);
  }
  if (last_hour != nowr.Hour()) {
    last_hour = nowr.Hour();
    //Serial.println("Checking if time is in check");
    strDateTime dateTime = NTPch.getNTPtime(-3.0, 0);
    if (dateTime.valid && (dateTime.minute != nowr.Minute()))
      updateRTC = true;
  }
}

void setup() {

  Serial.begin(115200);
  Wire.begin(D2, D1);
  setupPins();

  Serial.setDebugOutput(true);

  if (!SPIFFS.begin()) {
    return;
  }

  setupWifi();

  setupOTA();

  Rtc.Begin();
  setupRTC();


  loadInternalConfigs();

  setupStaticFiles();

  server.on("/lightdata", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    StaticJsonBuffer<200> jsonBuffer;
    File configFile = SPIFFS.open("/lightData.json", "r");
    while (configFile.available()) {
      response->print((char)configFile.read());
    }
    configFile.close();
    request->send(response);
  });

  server.on("/updatertc", HTTP_GET, [](AsyncWebServerRequest *request) {
    updateRTC = true;
    request->redirect("/");
  });

  server.on("/timeval", HTTP_GET, [](AsyncWebServerRequest *request) {
    char buff[20];
    sprintf(buff, "%f", horarioToDec(Rtc.GetDateTime()));
    request->send(200, "text/plain", buff);
  });

  AsyncCallbackJsonWebHandler *lightDataHandler = new AsyncCallbackJsonWebHandler("/lightdata", [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &jsonConfig = json.as<JsonObject>();
    if (!jsonConfig.success()) {
      //Serial.println("Failed to open/parse obj");
      request->send(500);
    } else {
      jsonConfig.printTo(Serial);
      loadJsonConfig(jsonConfig, request->hasParam("save"));
      request->send(200, "text/plain", "Uhuul, it works!");
    }
  });

  server.addHandler(lightDataHandler);

  server.begin();

  setupTCPServer();
}

unsigned long lastLoop = 0;

void loop() {
  unsigned long now = millis();
  if(now > lastLoop + 1000){
    lastLoop = now;
    char timestr[9];
    RtcDateTime nowr = Rtc.GetDateTime();
    timeCheck(nowr);
    timeToString(timestr, nowr);
    Serial.write(timestr);
    //Serial.println();
    if(isManual){
      for (int i = 0; i < color_number; i++) {
        if (alarm){
          analogWrite(color_pins[i], 0);
        } 
        //Serial.printf("MANUAL | Color %s value %d \n", color_names[i], manualBrightness[i]);
      }
      if(millis() > manualExpiresAt) isManual = false;
    } else {
      for (int i = 0; i < color_number; i++) {
        int brightness = getLightValue(nowr, color_params[i]);
        if (alarm) brightness = 0;
        analogWrite(color_pins[i], brightness);
        //Serial.printf("Color %s value %d \n", color_names[i], brightness);
      }
    }
    
    if (alarm) {
      //Serial.println("Alarm is up!");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    } else {
      digitalWrite(LED_BUILTIN, 1);
    }
  }
  ArduinoOTA.handle();
}