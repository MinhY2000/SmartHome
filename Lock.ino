#include <MFRC522.h>
#include <LiquidCrystal_I2C.h> 
#include <Keypad.h> 
#include <SPI.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

#define SS_PIN 5
#define RST_PIN 0
#define buzzerPin 15
#define redLed 2
#define ELock 16
#define EEPROM_SIZE 64

WiFiClient client;
PubSubClient mqtt_client(client);

const char *ssid = "Nhat";        // Name Wifi
const char *Pass = "khongcopass"; // Password Wifi
const char *mqtt_server = "test.mosquitto.org";
const char *mqtt_id = "esp32";
int failPass;

MFRC522 mfrc522(SS_PIN, RST_PIN);
int id_recent[4], numberMemory;
int numberUsedMemory = 2 ;
int address_second = 5;
const char initial_password[] = "223356";   
const char pass_toAddCard[] = "AACC"; 
const char pass_toDeleteCard[] = "BBCC"; 
char password[6];
char pass[4];                                                                       // Mật khẩu nhập từ bàn phím
boolean RFIDMode = false;  
unsigned long time_delay;

char key_pressed = 0;                                                                       // Biến lưu giữ phím được nhấn trên bàn phím
uint8_t i, j = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
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
byte rowPins[rows] = {32, 33, 25, 26}; 
byte columnPins[columns] = {27, 14, 12, 13};
Keypad keypad_key = Keypad(makeKeymap(keys), rowPins, columnPins, rows, columns);      

void setup() 
{
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  setup_wifi();
  pinMode(buzzerPin, OUTPUT);    
  pinMode(ELock, OUTPUT);  
  pinMode(redLed, OUTPUT);                                                                          // Đưa Servo motor về góc quay 0 độ ở vị trí ban đầu
  lcd.init();                                                                               // Khởi tạo màn hình LCD
  lcd.backlight();                                                                          // Bật đèn nền của màn hình LCD
  SPI.begin();                                                                              // Khởi tạo giao tiếp SPI
  mfrc522.PCD_Init();   
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("    WelCome    ");
  mqtt_client.setServer(mqtt_server, 1883);
  mqtt_client.setCallback(callback);
  Serial.println("Connecting to mqtt... ");
  while (!mqtt_client.connect(mqtt_id))
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to mqtt ");                                                                  // Khởi tạo MFRC522      
}
void setup_wifi() 
{
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, Pass);
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
}
void resetPass(char p[], int size)
{
  for(int pos = 0; pos < size ; pos ++)
  {
    p[pos] = 0;
  }
}
void beep() 
{
  digitalWrite(buzzerPin, HIGH);                                                            // Kích hoạt buzzer
  delay(300);                                                                               // Delay 100ms
  digitalWrite(buzzerPin, LOW);                                                             // Tắt buzzer
}
void tizz() 
{
  digitalWrite(buzzerPin, HIGH);
  delay(100);
  digitalWrite(buzzerPin, LOW);
}
void checkPassword()
{
  int a = 30;
  if (!(strncmp(password, initial_password, 4))) 
  {
    lcd.setCursor(0, 1);
    lcd.print(" Mat Khau Dung"); 
    digitalWrite(ELock, HIGH);
    Serial.println("Elock HIGH");
    beep();
    if(isMasterCard())
    {
      lcd.setCursor(1, 0);
      lcd.print("Had Master Card");
      delay(3000);
    }
    else
    {
      lcd.setCursor(1, 0);
      lcd.print("ProcessAddCard");
      delay(1000);
      while(a != 0)
      {
        delay(100);
        addIDCard(1);
        a -= 1;
        Serial.println("a1 ="+ String(a));
      }
    }
    failPass = 0;   
    digitalWrite(ELock, LOW);
    Serial.println("Elock LOW");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("    WelCome    ");  
  } 
  else 
  {
    failPass ++;
    Serial.println("Number Fail: "+ String(failPass));
    lcd.setCursor(0, 1);
    lcd.print("  Sai Mat Khau");
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(redLed, HIGH);
    delay(1500);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(redLed, LOW);
    if (failPass == 3)
    {
      Serial.print("Theft Warning!");
      bool passPassword = false;
      String t = (String)passPassword;
      mqtt_client.publish("Notify", t.c_str()); 
      lcd.setCursor(1, 0);
      lcd.print("    Warning    "); 
      failPass=0;
      delay(3000);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("    WelCome    "); 
    }
    else
    {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("    WelCome    "); 
    }
  }
  resetPass(password,6);
  i = 0;
}
int checkIDCard ()
{
  int p = 1;
  int l, k, tempValueEEPROM = 0;
  int id_eeprom[4], fail;
  while(p < numberMemory) 
  {
    l = p + 4; 
    for(p; p < l; p++)
    {
      tempValueEEPROM= EEPROM.read(p);
      if(k <= 3) 
      {
        id_eeprom[k] = tempValueEEPROM;
      }
      k++;
    }
    if(id_recent[0] == id_eeprom[0] && id_recent[1] == id_eeprom[1] && id_recent[2] == id_eeprom[2] && id_recent[3] == id_eeprom[3]) 
    {
      return p;
      fail=100;
      break;
    }else fail--;
    k = 0;
  }
  if(fail != 100) return 0;
}
void addIDCard(int address)
{
  if (!mfrc522.PICC_IsNewCardPresent()) {return;}
  if (!mfrc522.PICC_ReadCardSerial()) {return;}
  for (byte n = 0; n < 4; n++) 
  { 
    Serial.print(mfrc522.uid.uidByte[n] < 0x10 ? " " :" "); 
    Serial.print(mfrc522.uid.uidByte[n], DEC);         
    // id_recent[i] = mfrc522.uid.uidByte[i];
    EEPROM.write(address, mfrc522.uid.uidByte[n]);
    EEPROM.commit();
    address ++;
    Serial.println("EEPROM : " + String(EEPROM.read(n+1))); 
  }
  numberMemory += 4;
  EEPROM.write(0, numberMemory);
  lcd.setCursor(0,1);
  lcd.print("   DANG LUU...  "); 
  delay(1000);
}
void checkPass_toAddOrDeleteCard()
{
  int a = 30;
  if (!(strncmp(pass, pass_toAddCard, 4))) 
  {
    lcd.setCursor(1, 0);
    lcd.print("PassToAddCard");
    delay(1000);
    while(a != 0)
    {
      delay(100);
      addIDCard(9);
      a -= 1;
      Serial.println("a ="+ String(a));
    }
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("    WelCome    ");
  } 
  else if (!(strncmp(pass, pass_toDeleteCard, 4))) 
    {
      lcd.setCursor(0, 0);
      lcd.print("PassToDeleteCard");
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("    WelCome    "); 
    } 
    else 
    {
      lcd.setCursor(5, 1);
      lcd.print("Fail");
      delay(1000);
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("    WelCome    ");
    }
  resetPass(pass,4);
  j = 0;
  RFIDMode = false;
  digitalWrite(ELock, LOW);
  Serial.println("Elock LOW");
}
void MasterIDCard() 
{
  lcd.setCursor(2, 1);
  lcd.print("Master ID Card");
  digitalWrite(ELock, HIGH);
  Serial.println("Elock HIGH");
  beep();
  failPass = 0;
  RFIDMode =true;
  time_delay = millis();
}
bool isMasterCard() 
{
  int p = 1;
  int l, k, numberMasterCard, tempValueEEPROM = 0;
  int id_eeprom[4], fail;
  while(p < 9) 
  {
    l = p + 4; 
    for(p; p < l; p++)
    {
      tempValueEEPROM= EEPROM.read(p);
      if(k <= 3) 
      {
        id_eeprom[k] = tempValueEEPROM;
      }
      k++;
    }
    if(id_eeprom[0] == 0 && id_eeprom[1] == 0 && id_eeprom[2] == 0 && id_eeprom[3] == 0) 
    {
      numberMasterCard ++; 
      break;
    }
  }
  if(numberMasterCard == 0) { return false;}
  else return true;
} 
void loop() 
{
  mqtt_client.loop();
  numberMemory = EEPROM.length();
  key_pressed = keypad_key.getKey();
  if(RFIDMode)
  {
    if ( (unsigned long) (millis() - time_delay) < 5000)
    {
      if (key_pressed) 
      {
        pass[j++] = key_pressed;
        tizz();
        lcd.setCursor(j + 4, 1);
        lcd.print("*");                                                            // Hiển thị dấu '*' trên màn hình LCD khi nhập mật khẩu
      }
      if (j == 4) 
      {
        delay(200);
        checkPass_toAddOrDeleteCard();
        // Serial.println("j ="+ String(j));
        // Serial.println("RFIDmode: "+ String(RFIDMode));
      }
    }
    else 
    {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("    WelCome    ");
      RFIDMode = false;
      digitalWrite(ELock, LOW);
      Serial.println("Elock LOW");
    }
  }
  else
  {
    if (key_pressed) 
    {
      password[i++] = key_pressed;
      tizz();
      lcd.setCursor(i + 4, 1);
      lcd.print("*");                                                                       // Hiển thị dấu '*' trên màn hình LCD khi nhập mật khẩu
    }
    else
    {
      if (!mfrc522.PICC_IsNewCardPresent()) {return;}
      if (!mfrc522.PICC_ReadCardSerial()) {return;}
      for (byte m = 0; m < 4; m++) 
      { 
        Serial.print(mfrc522.uid.uidByte[m] < 0x10 ? " " :" "); 
        Serial.print(mfrc522.uid.uidByte[m], DEC);         
        id_recent[m] = mfrc522.uid.uidByte[m];
      }
      Serial.println("checkIDCard: "+ String(checkIDCard())); 
      switch (checkIDCard())
      {
        case 5:
          MasterIDCard();
          break;
        case 9:
          MasterIDCard();
          break;
        case 0:
          failPass++;
          Serial.println("Number Fail: "+ String(failPass));
          lcd.setCursor(0, 1);
          lcd.print(" The Khong Dung");
          digitalWrite(buzzerPin, HIGH);
          digitalWrite(redLed, HIGH);
          delay(1500);
          digitalWrite(buzzerPin, LOW);
          digitalWrite(redLed, LOW);
          if (failPass == 3)
          {
            Serial.print("Theft Warning!");
            bool passPassword = false;
            String t = (String)passPassword;
            mqtt_client.publish("Notify", t.c_str()); 
            lcd.setCursor(1, 0);
            lcd.print("    Warning    "); 
            failPass=0;
            delay(3000);
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print("    WelCome    "); 
          }
          else
          {
            lcd.clear();
            lcd.setCursor(1, 0);
            lcd.print("    WelCome    ");  
          }
          break;
        default:
          lcd.setCursor(2, 1);
          lcd.print("Slave ID Card");
          digitalWrite(ELock, HIGH);
          Serial.println("Elock HIGH");
          beep();
          failPass = 0;
          delay(5000);
          digitalWrite(ELock, LOW);
          Serial.println("Elock LOW");
          lcd.clear();
          lcd.setCursor(1, 0);
          lcd.print("    WelCome    ");
          break;
      }
    }
    if (i == 6) 
    {
      delay(200);
      checkPassword();
    }
  }
}