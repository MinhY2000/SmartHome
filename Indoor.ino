#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string.h>
#include <ESP32Servo.h>
#include "DHT.h"
#include <Wire.h>

#define DHTPIN 14
#define MQ2_A 35
#define LED_LIVING 27
#define LED_BED 26
#define LED_KITCHEN 32
#define BUTTON_BED 13
#define BUTTON_LIVING 34
#define BUTTON_KITCHEN 39
#define BUTTON_GARAGE 36
#define DOOR_GARAGE 23
#define PIR 12
#define LED_INBATH 25
#define LED_USING 33
#define FAN 3
#define Speed_FAN 19
#define DHTTYPE DHT11  

TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;
PubSubClient mqtt_client(client);

Servo garageServo;

const char *ssid = "Béo ><";        // Name Wifi
const char *password = "lethuydung";
// const char *ssid = "ROSE 501";        // Name Wifi
// const char *password = "0908812815";
// const char *ssid = "Nhat";        // Name Wifi
// const char *password = "khongcopass"; // Password Wifi
const char *mqtt_server = "test.mosquitto.org";
const char *mqtt_id = "esp32";

unsigned long lastMsg = 0;
float  gas, temp, humi,  TimeNoOneInside, speed_fan;
bool statusBed, statusLiving, statusKitchen, statusDoor ;
bool isStatusDoor = false;

void setup() 
{
  Serial.begin(115200);
  setup_wifi();
  speed_fan=50;
  
  pinMode(LED_USING, OUTPUT);
  pinMode(LED_INBATH, OUTPUT);  
  pinMode(PIR, INPUT_PULLUP);
  pinMode(FAN, OUTPUT);
  pinMode(Speed_FAN, OUTPUT);

  pinMode(MQ2_A, INPUT);
  pinMode(LED_BED, OUTPUT);
  pinMode(LED_LIVING, OUTPUT);
  pinMode(LED_KITCHEN, OUTPUT);

  pinMode(BUTTON_BED, INPUT);  
  pinMode(BUTTON_KITCHEN, INPUT);
  pinMode(BUTTON_LIVING, INPUT);
  pinMode(BUTTON_GARAGE, INPUT); 

  garageServo.attach(DOOR_GARAGE);

  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);

  Serial.println("Connecting to mqtt... ");
  while (!mqtt_client.connect(mqtt_id))
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to mqtt ");
  mqtt_client.subscribe("LED1");
  mqtt_client.subscribe("LED2");
  mqtt_client.subscribe("LED3");  
  mqtt_client.subscribe("GARA");  
  dht.begin();

  xTaskCreatePinnedToCore(
                    Task_Control_Bath,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    2,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500); 

  xTaskCreatePinnedToCore(
                    Task_Display_Temp_Humi_Gas,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    4,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 1 */
    delay(500); 

  xTaskCreatePinnedToCore(
                    Task_Control_Button_LED,   /* Task function. */
                    "Task3",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task3,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 0 */                  
  delay(500); 
}
void setup_wifi() 
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("\n");
  Serial.print("Recived from: ");
  Serial.println(topic);
  Serial.print("Message: ");
  String messageTemp ;
  for (size_t i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  if(String(topic))
  {
    if (String(topic) == "LED1") 
    {
      if (messageTemp == "1") 
      {
        Serial.println("on");
        digitalWrite(LED_BED, HIGH);
      }
      else if (messageTemp == "0") 
      {
        Serial.println("off");
        digitalWrite(LED_BED, LOW);
      }
    }
    if (String(topic) == "LED2") 
    {
      if (messageTemp == "1") 
      {
        Serial.println("on");
        digitalWrite(LED_LIVING, HIGH);
      }
      else if (messageTemp == "0") 
      {
        Serial.println("off");
        digitalWrite(LED_LIVING, LOW);
      }
    }
    if (String(topic) == "GARA") 
    {
      if (messageTemp == "1") 
      {
        Serial.println("on");
        OpenDoor();
      }
      else if (messageTemp == "0") 
      {
        Serial.println("off");
        CloseDoor();
      }
    }
    else if (String(topic) == "LED3") 
    {
      if (messageTemp == "1") 
      {
        Serial.println("on");
        digitalWrite(LED_KITCHEN, HIGH);
      }
      else if (messageTemp == "0") 
      {
        Serial.println("off");
        digitalWrite(LED_KITCHEN, LOW);
      }
    }
  }
}
void OpenDoor()
{
  isStatusDoor=true;
  for(int posDegrees = 0; posDegrees <= 183; posDegrees++) 
  {
    garageServo.write(posDegrees);
    delay(5);
  }
}
void CloseDoor()
{
  isStatusDoor=false;
  for(int posDegrees = 183; posDegrees >= 0; posDegrees--) {
    garageServo.write(posDegrees);
    delay(5);
  }
}
void sendMQTTvalues(float temp, float hum, float gas)
{
  StaticJsonDocument<256> doc;

  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["gas"] = gas;

  char buff[256];
  serializeJson(doc, buff);
  mqtt_client.publish("ESPValues",buff);
}
void Task_Control_Bath( void * pvParameters )
{
  for(;;)
  {
      if (digitalRead(PIR)==1)
    {
      digitalWrite(LED_INBATH, HIGH);
      digitalWrite(LED_USING, HIGH);
      digitalWrite(FAN, HIGH);
      analogWrite(Speed_FAN, speed_fan);
      TimeNoOneInside =0;
      delay(400);
      for(int timedelay = 0 ; timedelay < 20 ;timedelay++)
      {
        if (digitalRead(PIR)==1)
        {
          timedelay=0;
        }
        Serial.print("\t");
        Serial.print("timedelay= "+(String)timedelay);
        delay(1000);
      }
    }
    else
    {
      digitalWrite(LED_INBATH, LOW);
      TimeNoOneInside += 1;
      Serial.println(TimeNoOneInside);
      delay(1000);
      if (TimeNoOneInside == 7)
      {
        digitalWrite(LED_USING, LOW);
        digitalWrite(FAN, LOW);
      }
    }
  } 
}
void Task_Display_Temp_Humi_Gas( void * pvParameters )
{
  mqtt_client.loop();
  for(;;)
  {
    // const TickType_t xDelay = 500 / portTICK_PERIOD_MS;
    temp = dht.readTemperature();
    humi = dht.readHumidity();
    gas = analogRead(MQ2_A); 

    Serial.print("\n");
    Serial.print("Humidity: " + String(humi) + "%");
    Serial.print("\t");
    Serial.print("Temperature:" + String(temp) + " C");
    Serial.print("\t");
    Serial.print("Gas:" + String(gas) + "ppm");

    sendMQTTvalues(temp, humi, gas);
    delay(3000);
    // vTaskDelay( xDelay );
  }
}
void Task_Control_Button_LED( void * pvParameters )
{
  for(;;)
  {
    mqtt_client.loop();
    if (digitalRead(BUTTON_BED)==1)
    {
      statusBed= !statusBed;
      String t =  (String)statusBed;
      mqtt_client.publish("Button1", t.c_str());
      delay(400);
    }
    if (digitalRead(BUTTON_LIVING)==1)
    {
      statusLiving= !statusLiving;
      String t =  (String)statusLiving;
      mqtt_client.publish("Button2", t.c_str());
      delay(400);
    }
    if (digitalRead(BUTTON_KITCHEN)==1)
    {
      statusKitchen= !statusKitchen;
      String t =  (String)statusKitchen;
      mqtt_client.publish("Button3", t.c_str());
      delay(400);
    }
    if (digitalRead(BUTTON_GARAGE)==1)
    {
      isStatusDoor= !isStatusDoor;
      String t =  (String)isStatusDoor;
      mqtt_client.publish("Garage", t.c_str());
      delay(400);
    }
  }
}
void loop() 
{
  
}

   