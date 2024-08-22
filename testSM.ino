#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <Servo.h>
#include "DHT.h"

WiFiClient espClient;
PubSubClient client(espClient); 

const char* ssid = "Nhat";
const char* password = "khongcopass";
const char* mqtt_server= "test.mosquitto.org" ;




#define DHTTYPE DHT11   
#define DHTPIN 14
#define LED_LIVING 27
#define LED_BED 26
#define LED_inBATH 25
#define LED_SomeoneUsing 21
#define LED_KITCHEN 24
#define Buzzer 23
#define Fan 10
#define BUTTON_BED 12
#define BUTTON_LIVING 15
#define BUTTON_KITCHEN 16
#define BUTTON_DOOR 17
#define MQ2_A 35
#define PIR 11

// WidgetLED LED_ON_APP(V4); 
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27,16,2);
Servo Motor_Door;
// int Motor_pin = 10;

int button, gas, temp, humi, motor, NoOneInside;

const byte rows = 4; 
const byte columns = 4; 
char key = 0;
char keys[rows][columns] =
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};
byte rowPins[rows] = {19, 18, 5, 17}; 
byte columnPins[columns] = {16, 4, 0, 15};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, columnPins, rows, columns);
int failPass;
const char password[] = "253689"; 
char enteredPassword[6];        
bool isLocked = true;         

void setup() 
{
  Serial.begin(115200);
  Serial.print("Connecting to wifi....");
  Wifi.begin(ssid, password);
  Wifi.reconnect();
  while (!Wifi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(",");
  }
  Serial.print("Connected to wifi ");
  Serial.print(ssid);
  Serial.print("IP Address: ");
  Serial.print(Wifi.localIP());;

  








  // put your setup code here, to run once:
  pinMode(LED_LIVING, OUTPUT);
  pinMode(LED_inBATH, OUTPUT);
  pinMode(LED_KITCHEN, OUTPUT);
  pinMode(LED_SomeoneUsing, OUTPUT);
  pinMode(LED_BED, OUTPUT);
  pinMode(Fan, OUTPUT);
  pinMode(MQ2_A, INPUT);
  pinMode(BUTTON_BED, INPUT);
  pinMode(BUTTON_DOOR, INPUT);
  pinMode(BUTTON_KITCHEN, INPUT);
  pinMode(BUTTON_LIVING, INPUT);

  
  lcd.init();
  lcd.backlight();
  //LCD Password
  lcd.setCursor(0,0);
  lcd.print("Please press A ");
  lcd.setCursor(0,1);
  lcd.print("to enter Pass !");
  //LCD temp humi gas
  // lcd.setCursor(0,0);
  // lcd.print("Humi:");
  // lcd.setCursor(8,0);
  // lcd.print("Temp:");
  // lcd.setCursor(0,1);
  // lcd.print("Gas:");
  dht.begin();
  Motor_Door.attach(10);
}

BLYNK_WRITE(V3) 
{
  button = param.asInt();
  digitalWrite(LED_BED, button);
  Blynk.virtualWrite(V4,digitalRead(LED_BED));
}

void resetPassword() 
{
  memset(enteredPassword, 0, sizeof(enteredPassword)); 
}
bool checkPassword() 
{
  if (strcmp(enteredPassword, password) == 0)
  {
    return true;
  } else return false;
}
void controlDoor() 
{
  if (isLocked)
  {
    lcd.clear();
    lcd.print("Locked");
    motor = -90;
    Motor_Door.write(motor);
    delay(20);
  }
  else
  {
    motor = 90;
    Motor_Door.write(motor);
    delay(20);
  }
}
void Warning_Fail_Pass()
{

}
void Warning_Gas()
{

}
bool Is_People_Doing()
{
  if (digital.read(PIR) == HIGH)
  {
    return true;
  } else return false
}

void loop() 
{

  // put your main code here, to run repeatedly:
  Blynk.run();
  
  void Task_Display_Temp_Humi_Gas(void *param)
  {
    temp = dht.readTemperature();
    humi = dht.readHumidity();
    gas = analogRead(MQ2_A);

    Blynk.virtualWrite(V2, gas);
    Blynk.virtualWrite(V0, temp);
    Blynk.virtualWrite(V1, humi);

    lcd.setCursor(5,0);
    lcd.print(String(humi)+"%");
    lcd.setCursor(13,0);
    lcd.print(String(temp)+"C");
    lcd.setCursor(4,1);
    lcd.print(String(gas)+"PPM"); 
  }

  void Task_Handle_Password(void *param)
  {
    key = keypad.getKey(); 
    if (key) 
    {
      if (key == 'A') 
      {
        lcd.clear();
        lcd.print("Enter password:");
        lcd.setCursor(0, 1); 
      } 
      if (key == 'B') 
      {
        lcd.clear();
        lcd.print("Password cleared");
        resetPassword();
        delay(1000);
        lcd.setCursor(0,0);
        lcd.print("Please press A ");
        lcd.setCursor(0,1);
        lcd.print("to enter Pass !");
      } 
      else if (key == 'D') 
      {
        if (checkPassword()) 
        {
          isLocked = false;
          controlDoor();
          failPass = 0;
          lcd.clear();
          resetPassword();
          lcd.print("Unlocked");
        }
        else
        {
          failPass++;
          lcd.clear();
          lcd.print("Fail Password");
          lcd.setCursor(0, 1); 
          lcd.print("Number Fail: "+ String(failPass));
          if (failPass == 3)
          {
            WarningBuzz();
            Serial.print("Theft Warning!");
            failPass=0;
            delay(2000);
            lcd.clear();
            resetPassword();
            lcd.print("Enter password:");
          }
          else
          {
            delay(2000);
            lcd.clear();
            resetPassword();
            lcd.print("Enter password:");
          }
        }
      } 
      else {
        if (enteredPassword[0] == '\0') {
          lcd.clear();
          lcd.print("Enter password:");
        }
        if (strlen(enteredPassword) < 6) {
          enteredPassword[strlen(enteredPassword)] = key;
          lcd.setCursor(strlen(enteredPassword)-1,1);
          lcd.print('*');
          Serial.print("\t");
          Serial.print(strlen(enteredPassword));
        }
      }
    }  
  }

  void Task_Control_Button_LED(void *paramm)
  {
    if (digitalRead(BUTTON_BED)==0)
    {
      digitalWrite(LED_BED, !digitalRead(LED_BED));
      Blynk.virtualWrite(V3,digitalRead(LED_BED));
      Blynk.virtualWrite(V4,digitalRead(LED_BED));
    }
    if (digitalRead(BUTTON_LIVING)==0)
    {
      digitalWrite(LED_LIVING, !digitalRead(LED_LIVING));
      Blynk.virtualWrite(V4,digitalRead(LED_LIVING));
    }
    if (digitalRead(BUTTON_KITCHEN)==0)
    {
      digitalWrite(led, !digitalRead(LED_KITCHEN));
      Blynk.virtualWrite(V4,digitalRead(LED_KITCHEN));
    }
    if (digitalRead(BUTTON_DOOR)==0)
    {
      isLocked = true;
      controlDoor();
    }
  }
  
  void Task_Control_Bath(void *param)
  {
    if (Is_People_Doing())
    {
      digital.Write(LED_inBATH, HIGH);
      digital.Write(LED_SomeoneUsing, HIGH);
      digital.Write(Fan, HIGH);
      NoOneInside =0;
      delay(10000);
    }
    else
    {
      digital.Write(LED_inBATH, LOW);
      NoOneInside += 1;
      delay(1000);
      if (NoOneInside == 5)
      {
        digital.Write(LED_SomeoneUsing, LOW);
        digital.Write(Fan, LOW);
      }
    }
  }

  //Serial Humi Gas Temp
  // Serial.print("\n");
  // Serial.print("Humidity: " + String(humi) + "%");
  // Serial.print("\t");
  // Serial.print("Temperature:" + String(temp) + " C");
  // Serial.print("\t");
  // Serial.print("Gas:" + String(gas) + "ppm");
  // Serial.print("\t");
  // Serial.print("LED:" + String(button));
  delay(100);
}

   