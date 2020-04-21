#include <ArduinoJson.h> //ArduinoJSON6
#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

DynamicJsonDocument TestSPIFF(2048);

const char* ssid = "SSID";                // WiFi SSID
const char* password = "PASSWORD";        // WiFi Password
const char* mqtt_server = "BROKER IP";  // IP Broker MQTT
const char* TopicUP = "SPIFF-UP";
const char* TopicDOWN = "SPIFF-DOWN";
const char* HostName = "SPIFF-TEST";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HostName);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  if (topic == TopicDOWN) {
    String messageTemp;
    for (int i = 0; i < length; i++) {
      messageTemp += (char)message[i];
    }
    Serial.print("New value : ");
    Serial.println(messageTemp);

    TestSPIFF["Demo"] = messageTemp;
    SaveSPIFF();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(HostName)) {
      Serial.println("connected");
      client.subscribe(TopicUP);
      client.subscribe(TopicDOWN);
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_BUILTIN, HIGH);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void LoadSPIFF() {
  //Load the Test.JSON file to an ArduinoJson file
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File test_file = SPIFFS.open("/Test.json", "r");
  if (!test_file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  //Reading of the JSON file
  String  test_string = test_file.readString();
  //Deserialization to a ArduinoJSON file
  deserializeJson(TestSPIFF, test_string);
  test_file.close();
  Serial.println("File Loaded");

  //Raw JSON string
  Serial.print("Raw : ");
  Serial.println(test_string);

  //ArduinoJSON
  Serial.print("ArduinoJSON : ");
  String output = TestSPIFF["Demo"];
  Serial.println(output);

  //MQTT Value 
  if (!client.connected()) {
    reconnect();
  }
  client.publish(TopicUP, output.c_str());

}

void SaveSPIFF() {
  String test_string;
  //Open the config.json file (Write Mode)
  File test_file = SPIFFS.open("/Test.json", "w");
  if (!test_file) {
    Serial.println("Failed to open file (Writing mode)");
    return;
  }
  //Serialize the ModulesInfos file to a JSON string
  serializeJson(TestSPIFF, test_string);

  //Save and close the JSON file
  if (test_file.println(test_string)) {
    Serial.println("File Saved");
    //client.publish(TopicDEBUG, config_string.c_str());
  } else {
    Serial.println("File write failed");
    //client.publish(TopicDEBUG, "File write failed");
  }
  test_file.close();

  //MQTT Raw JSON string
  if (!client.connected()) {
    reconnect();
  }
  client.publish(TopicUP, test_string.c_str());
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(5);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(LED_BUILTIN, OUTPUT);
  client.setCallback(callback);
  LoadSPIFF();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
