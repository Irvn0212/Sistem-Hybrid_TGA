#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>

//Set Komunikasi Serial
#define rx 14
#define tx 12
SoftwareSerial mySerial(rx, tx);

//Set Reset Nilai
#define tombolReset 2

//LCD
LiquidCrystal_I2C lcd1(0x27, 16, 2);
LiquidCrystal_I2C lcd2(0x26, 16, 2);
LiquidCrystal_I2C lcd3(0x25, 16, 2);

//Set WiFi
const char* ssid = "Irfan";
const char* password = "smart0212";

//Set MQTT
const char* mqttServer = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;

//Set ESPclient
WiFiClient espClient;
PubSubClient ESPClient(espClient);

//Penampung Data 
String v[3], i[3], p[3], Si[3];
String supply;

//Set kWh dan Biaya
float kWh1, kWh2, kWh3;
float tarif1, tarif2, tarif3;
float kWhTotal1, kWhTotal2, kWhTotal3;
float Rp1, Rp2, Rp3;

//Time
unsigned long waktu, last;
unsigned long interval = 5000;

//Etc. Parameter
int banyakBacaKiriman, banyakKirimMinta, tampilWifi;
int banyakData, setTampil;
int nilaiReset, banyakReset;

void hubungWifi(){
  delay(10);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(250);
    if(tampilWifi < 1){
      Serial.println("");
      Serial.print("Menghubungkan Ke: ");
      Serial.println(ssid);
      tampilWifi++;
    }
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.print("Berhasil Menghubungkan Dengan: ");
  Serial.println(ssid);
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.println();
  Serial.print("Pesan Terkirim [");
  Serial.print(topic);
  Serial.println("]");
}

void reconnect(){
  while(!ESPClient.connected()){
    Serial.println("Menghubungkan Ke MQTT");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if(ESPClient.connect(clientId.c_str())){
      Serial.println("Berhasil Terhubung Ke MQTT");
      //Tambahkan Client Publish
      //Tambahkan Client Subscribe
    }else{
      Serial.println();
      Serial.print("Gagal Menghubungkan Dengan MQTT, rc=");
      Serial.println(ESPClient.state());
      Serial.println("Menghubungkan Kembali Dalam 5 Detik");
      delay(5000);
    }
  }
}

void mqtt(){
  if(!ESPClient.connected()){
    reconnect();
  }
  ESPClient.loop();
  ESPClient.publish("SistemHibrid/Irfan/PLTS/V", v[0].c_str());
  ESPClient.publish("SistemHibrid/Irfan/PLN/V", v[1].c_str());
  ESPClient.publish("SistemHibrid/Irfan/BEBAN/V", v[2].c_str());

  ESPClient.publish("SistemHibrid/Irfan/PLTS/I", i[0].c_str());
  ESPClient.publish("SistemHibrid/Irfan/PLN/I", i[1].c_str());
  ESPClient.publish("SistemHibrid/Irfan/BEBAN/I", i[2].c_str());

  ESPClient.publish("SistemHibrid/Irfan/PLTS/P", p[0].c_str());
  ESPClient.publish("SistemHibrid/Irfan/PLN/P", p[1].c_str());
  ESPClient.publish("SistemHibrid/Irfan/BEBAN/P", p[2].c_str());

  ESPClient.publish("SistemHibrid/Irfan/SUPPLY", supply.c_str());
  
  ESPClient.publish("SistemHibrid/Irfan/PLTS/KWH", String(kWhTotal1).c_str());
  ESPClient.publish("SistemHibrid/Irfan/PLN/KWH", String(kWhTotal2).c_str());
  ESPClient.publish("SistemHibrid/Irfan/BEBAN/KWH", String(kWhTotal3).c_str());

  ESPClient.publish("SistemHibrid/Irfan/PLTS/RP", String(Rp1).c_str());
  ESPClient.publish("SistemHibrid/Irfan/PLN/RP", String(Rp2).c_str());
  ESPClient.publish("SistemHibrid/Irfan/BEBAN/RP", String(Rp3).c_str());
  //Tambahkan publish
}

void kirimMinta(){
  if(banyakKirimMinta < 1){
    char minta[] = "123#123#";
    Serial.println(minta);
    mySerial.print(minta);
    banyakKirimMinta++;
  }
}

void bacaKiriman(){
  String arrData[13];
  
  kirimMinta();
  if(banyakBacaKiriman < 1){
    String dataKiriman;

    //Melihat Ketersedian Data Yang Dikirimkan Oleh Arduino
    while(mySerial.available() > 0){
      dataKiriman += char(mySerial.read());
    }
    dataKiriman.trim();
    if(dataKiriman != ""){
      int index = 0;
      for(unsigned int i = 0; i < dataKiriman.length(); i++){
        char pemisah = '#';
        if(dataKiriman[i] != pemisah){
          arrData[index] += dataKiriman[i];
        }else{
          index++;
        }
        banyakData = index;
      }
    }
    if(banyakData == 13){
      v[0] = arrData[0]; i[0] = arrData[3]; p[0] = arrData[6]; Si[0] = arrData[10];
      v[1] = arrData[1]; i[1] = arrData[4]; p[1] = arrData[7]; Si[1] = arrData[11];
      v[2] = arrData[2]; i[2] = arrData[5]; p[2] = arrData[8]; Si[2] = arrData[12];
      supply = arrData[9];
        
      for(int i = 0; i < banyakData; i++){
        //Serial.println(arrData[i]);
        arrData[i] = "";
      }
    }
    //Menampilkan Isi Data Dari Arduino
    Serial.println(String("Isi: ") +dataKiriman);
    mySerial.flush();
    banyakBacaKiriman++;
  }
  banyakKirimMinta = 0;
}

void hitungKwh(){
  float wh1, wh2, wh3;

  wh1 = p[0].toFloat() * 5 / 3600;
  wh2 = p[1].toFloat() * 5 / 3600;
  wh3 = p[2].toFloat() * 5 / 3600;

  kWh1 = wh1 / 1000;
  kWh2 = wh2 / 1000;
  kWh3 = wh3 / 1000;

  kWhTotal1 += kWh1;
  kWhTotal2 += kWh2;
  kWhTotal3 += kWh3;
  
  //Biaya perkWh Untuk Instalasi 1300 VA dan 2200 VA adala Rp. 1.444,52
  tarif1 = kWh1 * 1444.52;
  tarif2 = kWh2 * 1444.52;
  tarif3 = kWh3 * 1444.52;

  Rp1 += tarif1;
  Rp2 += tarif2;
  Rp3 += tarif3;
}

void resetNilai(){
  if(banyakReset < 1){
    if(nilaiReset == LOW){
      kWhTotal1 = 0;
      kWhTotal2 = 0;
      kWhTotal3 = 0;
      Rp1 = 0;
      Rp2 = 0;
      Rp3 = 0;
      Serial.println("Reset");
    }
    banyakReset++;
  }
  if(nilaiReset == HIGH){
      banyakReset = 0;
  }
}

void tampilanAwalLcd(){
  //Tampilan LCD 1
  lcd1.setCursor(4, 0);
  lcd1.print("WELCOME");
  lcd1.setCursor(1, 1);
  lcd1.print("PREPARE SYSTEM");

  //Tampilan LCD 2
  lcd2.setCursor(3, 0);
  lcd2.print("TGA IRFAN");
  lcd2.setCursor(1, 1);
  lcd2.print("SISTEM HYBRID");

  //Tampilan LCD 3
  lcd3.setCursor(0, 0);
  lcd3.print("WiFi Tersambung");
  lcd3.setCursor(0, 1);
  lcd3.print(ssid);
}

void tampilanTegangan(){
  //Tampilan Tegangan PLTS pada LCD 1
  lcd1.setCursor(0, 0);
  lcd1.print("V PLTS: ");
  lcd1.setCursor(8, 0);
  lcd1.print(v[0]);
  lcd1.setCursor(14, 0);
  lcd1.print("V");

  //Tampilan Tegangan PLN pada LCD 2
  lcd2.setCursor(0, 0);
  lcd2.print("V PLN: ");
  lcd2.setCursor(8, 0);
  lcd2.print(v[1]);
  lcd2.setCursor(14, 0);
  lcd2.print("V");

  //Tampilan Tegangan BEBAN pada LCD 3
  lcd3.setCursor(0, 0);
  lcd3.print("V BEBAN: ");
  lcd3.setCursor(9, 0);
  lcd3.print(v[2]);
  lcd3.setCursor(14, 0);
  lcd3.print("V");
}

void tampilanArus(){
  //Tampilan Arus PLTS pada LCD 1
  lcd1.setCursor(0, 1);
  lcd1.print("I PLTS: ");
  lcd1.setCursor(8, 1);
  lcd1.print(i[0]);
  lcd1.setCursor(14, 1);
  lcd1.print(Si[0]);

  //Tampilan Arus PLN pada LCD 2
  lcd2.setCursor(0, 1);
  lcd2.print("I PLN: ");
  lcd2.setCursor(8, 1);
  lcd2.print(i[1]);
  lcd2.setCursor(14, 1);
  lcd2.print(Si[1]);

  //Tampilan Arus BEBAN pada LCD 3
  lcd3.setCursor(0, 1);
  lcd3.print("I BEBAN: ");
  lcd3.setCursor(9, 1);
  lcd3.print(i[2]);
  lcd3.setCursor(14, 1);
  lcd3.print(Si[2]);
}

void tampilanSupply(){
  //Tampilan Supply Beban pada LCD 3
  lcd3.setCursor(0, 1);
  lcd3.print("Supply: ");
  lcd3.setCursor(8, 1);
  lcd3.print(supply);
}

void tampilanDaya(){
  //Tampilan Daya PLTS Pada LCD 1
  lcd1.setCursor(0, 0);
  lcd1.print("P PLTS: ");
  lcd1.setCursor(8, 0);
  lcd1.print(p[0]);
  lcd1.setCursor(14, 0);
  lcd1.print("W");

  //Tampilan Daya PLN Pada LCD 2
  lcd2.setCursor(0, 0);
  lcd2.print("P PLN: ");
  lcd2.setCursor(8, 0);
  lcd2.print(p[1]);
  lcd2.setCursor(14, 0);
  lcd2.print("W");

  //Tampilan Daya BEBAN Pada LCD 3
  lcd3.setCursor(0, 0);
  lcd3.print("P BEBAN: ");
  lcd3.setCursor(8, 0);
  lcd3.print(p[2]);
  lcd3.setCursor(14, 0);
  lcd3.print("W");
}

void tampilanKwh(){
  String Spt[3];
  float Pt1=0, Pt2=0, Pt3=0;
  if(kWhTotal1 > 0.1){Pt1 = kWhTotal1; Spt[0]="kWh";}
  if(kWhTotal2 > 0.1){Pt2 = kWhTotal2; Spt[1]="kWh";}
  if(kWhTotal3 > 0.1){Pt3 = kWhTotal3; Spt[2]="kWh";}
  if(kWhTotal1 < 0.1){Pt1 = kWhTotal1*1000; Spt[0]="Wh";}
  if(kWhTotal2 < 0.1){Pt2 = kWhTotal2*1000; Spt[1]="Wh";}
  if(kWhTotal3 < 0.1){Pt3 = kWhTotal3*1000; Spt[2]="Wh";}
  //Tampilan kWh PLTS pada LCD 1
  lcd1.setCursor(0, 0);
  lcd1.print("Ptotal: ");
  lcd1.setCursor(9, 0);
  lcd1.print(Pt1);
  lcd1.setCursor(13, 0);
  lcd1.print(Spt[0]);

  //Tampilan kWh PLN pada LCD 2
  lcd2.setCursor(0, 0);
  lcd2.print("Ptotal: ");
  lcd2.setCursor(9, 0);
  lcd2.print(Pt2);
  lcd2.setCursor(13, 0);
  lcd2.print(Spt[1]);

  //Tampilan kWh PLTS pada LCD 1
  lcd3.setCursor(0, 0);
  lcd3.print("Ptotal: ");
  lcd3.setCursor(9, 0);
  lcd3.print(Pt3);
  lcd3.setCursor(13, 0);
  lcd3.print(Spt[2]);
}

void tampilanBiaya(){
  //Tampilan Biaya Listrik PLTS Pada LCD 1
  lcd1.setCursor(0, 1);
  lcd1.print("Biaya :Rp");
  lcd1.setCursor(9, 1);
  lcd1.print(Rp1);

  //Tampilan Biaya Listrik PLN Pada LCD 2
  lcd2.setCursor(0, 1);
  lcd2.print("Biaya :Rp");
  lcd2.setCursor(9, 1);
  lcd2.print(Rp2);
  
  /*//Tampilan Biaya Listrik BEBAN Pada LCD 3
  lcd3.setCursor(0, 1);
  lcd3.print("Biaya :Rp");
  lcd3.setCursor(9, 1);
  lcd3.print(Rp3);*/
}

/*
void setTampilLcd(){
  setTampil =! setTampil;
  if(setTampil == HIGH){
    lcd1.clear();
    lcd2.clear();
    lcd3.clear();
    tampilanTegangan();
    tampilanArus();
  }
  if(setTampil == LOW){
    lcd1.clear();
    lcd2.clear();
    lcd3.clear();
    tampilanKwh();
    tampilanBiaya();
    tampilanSupply();
  }
}
*/

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  pinMode(tombolReset, INPUT);

  hubungWifi();
  
  ESPClient.setServer(mqttServer, mqttPort);
  ESPClient.setCallback(callback);

  //Inisialisasi LCD
  lcd1.init();
  lcd2.init();
  lcd3.init();
  lcd1.backlight();
  lcd2.backlight();
  lcd3.backlight();
  lcd1.clear();
  lcd2.clear();
  lcd3.clear();

  tampilanAwalLcd();
  //Mengatur Tampilan LCD
  setTampil = LOW;

  tampilWifi = 0;
  banyakKirimMinta = 0;
  banyakBacaKiriman = 0;
  banyakReset = 0;
}

void loop() {
  waktu = millis();
  nilaiReset = digitalRead(tombolReset);
  resetNilai();

  if(waktu - last > interval){
    last = waktu;
    bacaKiriman();
    hitungKwh();
    mqtt();

    setTampil =! setTampil;
    if(setTampil == HIGH){
        lcd1.clear();
        lcd2.clear();
        lcd3.clear();
        tampilanTegangan();
        tampilanArus();
    }if(setTampil == LOW){
        lcd1.clear();
        lcd2.clear();
        lcd3.clear();
        tampilanSupply();
        tampilanKwh();
        tampilanBiaya();
    }
    
    banyakBacaKiriman = 0;
  }
}