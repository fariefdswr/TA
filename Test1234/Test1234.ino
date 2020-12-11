#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <dht.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Panasonic.h>

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRPanasonicAc ac(kIrLed);  // Set the GPIO used for sending messages.

//#define relay_lampu D6 //Pin Relay
//#define sensor_gerak D7 //Pin Sensor PIR
#define sensor_suhu 14 //Pin Sensor DHT11
dht DHT;

void printState() {
  // Display the settings.
  Serial.println("Panasonic A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kPanasonicAcStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

const char* ssid = "fariefdswr";
const char* password =  "pitak1234";
const char* mqttServer = "broker.mqttdashboard.com";
const int mqttPort = 8000;
const char* mqttUser = "";
const char* mqttPassword = "";
String idClient = "clientId-gauBug1miS";

WiFiClient  quantumSensor;
PubSubClient client(quantumSensor);

void setup() {
  EEPROM.begin(1024);
  Serial.begin(115200);
  Wire.begin(D4,D3);
  lcd.init(); 
  ac.begin();
    
//  pinMode(relay_lampu,OUTPUT);
//  pinMode(sensor_gerak,INPUT);
  
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  tampilan_awal();
  koneksi_wifi();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("-----------------------");
  Serial.print("Pesan : ");
  String pesan;
  for (int i = 0; i < length; i++) {
  pesan += ((char)payload[i]);
  }
  int pesan_temp=pesan.toInt();
  EEPROM.write(3, pesan_temp);
  EEPROM.commit();
  Serial.println(pesan_temp);
  Serial.println("-----------------------");
  lcd.setCursor(0, 0);
  lcd.print("Perbaruhi");
  lcd.setCursor(0, 1);
  lcd.print("Temp Optimal:");
  lcd.print(pesan_temp);
  delay(1000);
  lcd.clear();
}

void loop() {
  int hum=EEPROM.read(1);
  int temp=EEPROM.read(2);
  int temp_optimal=EEPROM.read(3);

  client.loop();
  client.subscribe("quantum/temp_optimal");

//  automasi_lampu();
  pengaturan_suhu();
//  kirim_data(hum,temp,temp_optimal);
}

void koneksi_wifi(){
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("..");
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Wifi");
    lcd.setCursor(0, 1);
    lcd.print("Menghubungkan");
    delay(3000);
    lcd.clear();
//    automasi_lampu();
    pengaturan_suhu();
  }
  Serial.println("Wifi berhasil terhubung");
  Serial.print("IP Address : ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 0);
  lcd.print("Wifi Terhubung");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP :");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(3000);
  lcd.clear();
  if(!client.connected()){
    koneksi_mqtt();
  }
}

void koneksi_mqtt() {
    while (!client.connected()) {
    Serial.println("Menghubungkan ke MQTT...");
    idClient += String(random(0xffff), HEX);
    if (client.connect(idClient.c_str(), mqttUser, mqttPassword )) {
      Serial.println("Terhubung ke MQTT Server");
      lcd.setCursor(0, 0);
      lcd.print("MQTT Terhubung");
      delay(3000);
      lcd.clear();
    } else {
      Serial.print("Gagal terhubung ke MQTT dengan status ");
      Serial.println(client.state());
      lcd.setCursor(0, 0);
      lcd.print("MQTT");
      lcd.setCursor(0, 1);
      lcd.print("Menghubungkan");
      delay(2000);
      lcd.clear();
//      automasi_lampu();
      pengaturan_suhu();
    }
   }
}

void tampilan_awal(){  
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Monitoring Suhu");
  lcd.setCursor(0, 1);      
  //lcd.print("    Quantum");
  delay(3000);
  lcd.clear();
}

//void automasi_lampu(){
//  long gerak = digitalRead(sensor_gerak);
//    if(gerak == HIGH){
//      digitalWrite (relay_lampu, HIGH);
//      Serial.println("Ada Pergerakan");
//      lcd.setCursor(0, 0);
//      lcd.print("Ada Pergerakan");
//      lcd.setCursor(0, 1);
//      lcd.print("Lampu Hidup");
//      delay(2000);
//      lcd.clear();
//    }else {
//      digitalWrite (relay_lampu, LOW);
//      Serial.println("Tidak Ada Pergerakan");
//    }
//}

void tampilan(int hum, int temp, int temp_optimal, int temp_ac){
  Serial.print("Humidity : ");
  Serial.println(hum);
  Serial.print("Temperature : ");
  Serial.println(temp);
  Serial.print("Set Temperatur Optimal : ");
  Serial.println(temp_optimal);

  //Tampilkan ke LCD
  byte derajat = B11011111;
  lcd.setCursor(0, 0);
  lcd.print("T.O:");
  lcd.print(temp_optimal);
  lcd.write(derajat);
  lcd.print(" T.AC:");
  lcd.print(temp_ac);
  lcd.write(derajat);
  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(hum);
  lcd.print("%");
  lcd.print(" Temp:");
  lcd.print(temp);
  lcd.write(derajat);
  delay(3000);
  lcd.clear();
}

void pengaturan_suhu(){
  DHT.read11(sensor_suhu);
  int hum = DHT.humidity;
  int temp = DHT.temperature;
  int temp_optimal=EEPROM.read(3);
  int temp_ac=EEPROM.read(4);

  if (temp_ac > 30 || temp_ac < 16){
          temp_ac = temp_optimal;
  } else {
      if (temp > temp_optimal){
            if(temp_ac > 16){
                temp_ac = temp_ac-1;
              } 
                remote_ac(temp_ac);
                
      } else if (temp < temp_optimal){
            if(temp_ac < 30){
                temp_ac = temp_ac+1;
              } 
                remote_ac(temp_ac);
      }
 }
 
    EEPROM.write(1, hum);
    EEPROM.write(2, temp);
    EEPROM.write(3, temp_optimal);
    EEPROM.write(4, temp_ac);
    EEPROM.commit();
    
    tampilan(hum, temp, temp_optimal, temp_ac);
}

void remote_ac(int a){
  ac.setModel(kPanasonicRkr);
  ac.on();
  ac.setFan(kPanasonicAcFanAuto);
  ac.setMode(kPanasonicAcCool);
  ac.setTemp(a);
  ac.setSwingVertical(kPanasonicAcSwingVAuto);
  ac.setSwingHorizontal(kPanasonicAcSwingHAuto);
  #if SEND_PANASONIC_AC
  Serial.println("Sending IR command to A/C ...");
  ac.send();
  #endif  // SEND_PANASONIC_AC
  printState();
  lcd.print("Memperbaruhi");
  lcd.setCursor(0, 1);
  lcd.print("Temp. AC : ");
  lcd.print(a);
  lcd.setCursor(0, 2);
  lcd.print(a);
  delay(3000);
  lcd.clear();
}

//void kirim_data(int hum, int temp, int temp_optimal){
//  HTTPClient http; 
//
//  String s_hum, s_temp,  s_data_suhu_set, postData, putData;
//  s_hum = String(hum); //convert to string
//  s_temp = String(temp); //convert to string
//  s_data_suhu_set = String(temp_optimal); //convert to string
//  
//   //Post dan Put Data
//  postData = "humidity=" + s_hum + "&temperature=" + s_temp ; 
//  putData = "status=" + s_data_suhu_set ; 
//
//  http.begin("http://quantum-monitoring.herokuapp.com/index.php/rest_sensor");       //Specify request destination
//  http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
//
//  int httpCodePost = http.POST(postData);   //Send the request
//  String payloadPost = http.getString();    //Get the response payload
//  int httpCodePut = http.PUT(putData);      //Send the request
//  String payloadPut = http.getString();    //Get the response payload
//  
//  Serial.println(httpCodePost);   //Print HTTP return code
//  Serial.println(payloadPost);    //Print request response payload
//  Serial.println(httpCodePut);   //Print HTTP return code
//  Serial.println(payloadPut);    //Print request response payload

//  if(httpCodePost && httpCodePut == 200){
//    lcd.setCursor(0, 0);
//    lcd.print("Berhasil");
//    lcd.setCursor(0, 1);
//    lcd.print("Mengirim Data");
//    delay(2000);
//    lcd.clear();
//  } else {
//    lcd.setCursor(0, 0);
//    lcd.print("Gagal");
//    lcd.setCursor(0, 1);
//    lcd.print("Mengirim Data");
//    delay(2000);
//    lcd.clear();
//  }
//  http.end();  //Close connection
