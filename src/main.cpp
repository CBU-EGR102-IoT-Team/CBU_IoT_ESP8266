#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//WIFI settings
const char *ssid = "CBU";
const char *password = "";

//MQTT paramaters
const char *mqtt_server = "broker.hivemq.com"; //Change to match MQTT broker in use
size_t mqtt_port = 1883;
//MQTT Topic Structure: EGR102TA/ABox#/Sensor
String baseTopic = "cbuta";
String baseTopic1 = baseTopic + "0/"; //baseTopic + id

//functions
void wifi_setup();
void MQTTinit();
void MQTTreconnect();
void callback(char *topic, byte *payload, unsigned int length);
void mqttPost(String topic, String data);

//objects
WiFiClient espClient;
PubSubClient MQTTclient;

void setup() {
  Serial.begin(115200);
  wifi_setup();
  
  MQTTinit(); //init MQTT
  if (!MQTTclient.connected()) MQTTreconnect();
}

// Loop Monitor variables
bool serialPState = false;
int serialRequest = 0;
int serialPRequest = 0;
int loopCOunter = 0;
String input = "";

void loop() {
  //int input;

  //if serial is available print it back out to serial
  if(serialRequest != 0){
    if(Serial.available() > 0){
      //Serial.println(Serial.read());
      input += (char)Serial.read();
      serialPState = true;
    } else serialPState = false;

    //if the request was recieved
    if(serialPState == true && Serial.available() == 0){
      if(serialRequest == 1) //ID
      {
        mqttPost("cbuta/ID", input);
      }
      if(serialRequest == 2) //Volt1
      {
        mqttPost("cbuta/Volt1", input);
      }
      if(serialRequest == 3) //Volt2
      {
        mqttPost("cbuta/Volt2", input);
      }
      if(serialRequest == 4) //Light1
      {
        mqttPost("cbuta/Light1", input);
      }
      if(serialRequest == 5) //Light2
      {
        mqttPost("cbuta/Light2", input);
      }
      if(serialRequest == 6) //Distance1
      {
        mqttPost("cbuta/Distance1", input);
      }
      if(serialRequest == 7) //Distance2
      {
        mqttPost("cbuta/Distance2", input);
      }

      serialPRequest = serialRequest;
      serialRequest = 0; //this frees up the loop
      loopCOunter = 0;
      input = "";
    }

    //this is the arduino heart beat, if the arduino does not respond in this many loops, resend the request.
    if(loopCOunter == 25){
      loopCOunter = 0;
      serialRequest = 0;
    }
    if(Serial.available() == 0 && serialPState == false) loopCOunter++;
  }

  else{
    if(serialPRequest == 0) //Ask for ID
    {
      Serial.print("ID");
      serialRequest = 1;
    }
    if(serialPRequest == 1 || serialPRequest == 7) //ASK for Volt1
    {
      Serial.print("Volt1");
      serialRequest = 2;
    }
    if(serialPRequest == 2) //ASK for Volt2
    {
      Serial.print("Volt2");
      serialRequest = 3;
    }
    if(serialPRequest == 3) //ASK for Light1
    {
      Serial.print("Light1");
      serialRequest = 4;
    }
    if(serialPRequest == 4) //ASK for Light2
    {
      Serial.print("Light2");
      serialRequest = 5;
    }
    if(serialPRequest == 5) //ASK for Distance1
    {
      Serial.print("Distance1");
      serialRequest = 6;
    }
    if(serialPRequest == 6) //ASK for Distance2
    {
      Serial.print("Distance2");
      serialRequest = 7;
    }
  }
  delay(10);
}

/**
 * @brief Connects ESP to wifi with generic wifi setup process,
 *  Will restart ESP after 64 seconds
 */
void wifi_setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  uint8_t timer = 0;
  Serial.printf("Connecting to WiFi %s \n", ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    if (timer % 16 == 0 && timer > 15)
    Serial.printf("\n **Still trying to connect** , Timer: %i\n", timer);
    if (timer >= 64)
    break;
    timer++;
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
  }

  Serial.print("Connected, IP address:");
  Serial.print(WiFi.localIP());
  Serial.println();
}

/**
 * @brief Reconnects ESP to the MQTT broker
 * Note: Method is not called in main.cpp but is required by the MQTT library.
 */
void MQTTreconnect()
{
  while (!MQTTclient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ArduinoBox-";
    clientId += String(random(0xffff), HEX);

    if (MQTTclient.connect(clientId.c_str()))
    {
      Serial.println("connected");
      Serial.println("wifi: \t" + WiFi.localIP().toString());
      MQTTclient.publish("cbuta", "This is the payload");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" try again in 5 sec");
      delay(5000);
    }
  }
}

/**
 * @brief Used to read messages from the subscribed topic.
 *        The ESP is not subscribed to any topics, but MQTT library requires this method
 * 
 * @param topic ESP would be subscribed to.
 * @param payload message posted to the topic.
 * @param length of message.
 */
void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  String topic_str = "";

  for (size_t i = 0; i < length; i++)
    message += (char)payload[i];

  for (size_t i = 0; i < strlen(topic); i++)
    topic_str += topic[i];
}

/**
 * @brief Setup for MQTTclient
 */
void MQTTinit()
{
  MQTTclient.setClient(espClient);
  MQTTclient.setServer(mqtt_server, mqtt_port);
  MQTTclient.setCallback(callback);
}

void mqttPost(String topic, String data){
  if (!MQTTclient.connected()) MQTTreconnect();
    MQTTclient.publish(topic.c_str(), data.c_str()); //data is sent to RPi
}