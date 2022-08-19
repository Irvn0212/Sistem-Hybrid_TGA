#include <Filters.h>
#include <ACS712.h>
#include <SoftwareSerial.h>


//For Serial Comunication With NodeMCU
#define rx 12
#define tx 13
SoftwareSerial mySerial(rx, tx);

//For Sensor ZMPT101B
RunningStatistics stats1, stats2, stats3;
float windowLength = 40.0/50.0;
float intercept = -0.04;
float slope = 0.0964;
float vrms1, vrms2, vrms3;

//For Sensor ACS712
ACS712 ACS1(A5, 5.0, 1023, 185);
ACS712 ACS2(A5, 5.0, 1023, 185);
ACS712 ACS3(A5, 5.0, 1023, 185);
float irms1, irms2, irms3;
String I1 ="", I2 ="", I3 ="";

//For Calculate The Power
float P1, P2, P3;
float Q1, Q2, Q3;
float S1, S2, S3;
float cosPhi1, cosPhi2, cosPhi3;
float sinPhi1, sinPhi2, sinPhi3;

//For Set Supply LCD
String supply = "";

//For Relay Tegangan PLTS-PLN
#define relayPLTSPLN1 2
#define relayPLTSPLN2 3

//For Relay Inverter
#define relayInverter1 5
#define relayInverter2 6 

//For Indicator PLTS
#define relayIndikatorPLTS 4

//For Indicator PLN
#define relayIndikatorPLN 7

//
int banyakPeriksaPermintaan, transferData;
int panjangData;

//Time
unsigned long waktu, last;
unsigned long interval = 5000;


void bacaTegangan(){
  //Set Sensor ZMPT101B
  int zmpt1 = analogRead(A2);
  int zmpt2 = analogRead(A1);
  int zmpt3 = analogRead(A0);

  //Filter The Signal
  stats1.setWindowSecs(windowLength);
  stats1.input(zmpt1);
  stats2.setWindowSecs(windowLength);
  stats2.input(zmpt2);
  stats3.setWindowSecs(windowLength);
  stats3.input(zmpt3);

  //Calculate Vrms
  vrms1 = intercept+slope*stats1.sigma();
  vrms1 = vrms1*49.3231;
  vrms2 = intercept+slope*stats2.sigma();
  vrms2 = vrms2*49.3231;
  vrms3 = intercept+slope*stats3.sigma();
  vrms3 = vrms3*49.3231;

  //Filter The Value
  //Low Value
  if(vrms1<=5){vrms1 = 0;}
  if(vrms2<=5){vrms2 = 0;}
  if(vrms3<=5){vrms3 = 0;}
  //High Value
  if(vrms1>=220){vrms1 = 220;}
  if(vrms2>=220){vrms2 = 220;}
  if(vrms3>=220){vrms3 = 220;}
}

void bacaArus(){
  //Calculate Irms
  irms1 = ACS1.mA_AC();
  cosPhi1 = ACS1.getFormFactor();
  irms1 = irms1 - 70;
  I1 = "mA";
  irms2 = ACS2.mA_AC();
  cosPhi2 = ACS2.getFormFactor();
  irms2 = irms2 - 70;
  I2 = "mA";
  irms3 = ACS3.mA_AC();
  cosPhi3 = ACS3.getFormFactor();
  irms3 = irms3 - 70;
  I3 = "mA";

  //Filter The Value
  if(irms1<0){irms1 = 0;}
  if(irms2<0){irms2 = 0;}
  if(irms3<0){irms3 = 0;}
  
  //Convert Satuan
  if(irms1>100){irms1 = irms1/1000;I1 = "A";}
  if(irms2>100){irms2 = irms2/1000;I2 = "A";}
  if(irms3>100){irms3 = irms3/1000;I3 = "A";}
}

void hitungDaya(){
  float arus1, arus2, arus3;
  if(I1 = "A"){arus1 = irms1;}
  if(I2 = "A"){arus2 = irms2;}
  if(I3 = "A"){arus3 = irms3;}
  if(I1 = "mA"){arus1 = irms1/1000;}
  if(I2 = "mA"){arus2 = irms2/1000;}
  if(I3 = "mA"){arus3 = irms3/1000;}
  
  //Mencari Nilai SinPhi
  sinPhi1 = sqrt(1-pow(cosPhi1, 2));
  sinPhi2 = sqrt(1-pow(cosPhi2, 2));
  sinPhi3 = sqrt(1-pow(cosPhi3, 2));
  
  //Daya Semu
  S1 = vrms1*arus1;
  S2 = vrms2*arus2;
  S3 = vrms3*arus3;

  //Daya Aktif
  P1 = vrms1*arus1*cosPhi1;
  P2 = vrms2*arus2*cosPhi2;
  P3 = vrms3*arus3*cosPhi3;

  //Daya Reaktif
  Q1 = vrms1*arus1*sinPhi1;
  Q2 = vrms2*arus2*sinPhi2;
  Q3 = vrms3*arus3*sinPhi3;
}

void setRelayInverter(){
  if(vrms2 > 190){
    digitalWrite(relayInverter1, LOW);
    digitalWrite(relayInverter2, LOW);

    //On Indicator Running (Merah)
    //PLTS
    digitalWrite(relayIndikatorPLTS, HIGH);
    //PLN
    digitalWrite(relayIndikatorPLN, HIGH);
    Serial.println("GTI");
  }else{
    digitalWrite(relayInverter1, HIGH);
    digitalWrite(relayInverter2, HIGH);
    Serial.println("Stand Alone I");
    setRelayPLTSPLN();
  }
}

void setRelayPLTSPLN(){
  digitalWrite(relayPLTSPLN1, LOW);
  digitalWrite(relayPLTSPLN2, LOW);

  //Indicator Running PLTS 
  digitalWrite(relayIndikatorPLTS, HIGH);
  //Indicator Standby PLN
  digitalWrite(relayIndikatorPLN, LOW);
}

void setSupplyName(){
  float arus1, arus2, arus3;
  //Jadikan Satuan ke miliAmper Agar Lebih Sensitif dan Meningkatkan Akurasi.
  if(I1 = "A"){arus1 = irms1*1000;}
  if(I2 = "A"){arus2 = irms2*1000;}
  if(I3 = "A"){arus3 = irms3*1000;}
  if(I1 = "mA"){arus1 = irms1;}
  if(I2 = "mA"){arus2 = irms2;}
  if(I3 = "mA"){arus3 = irms3;}

  if((arus1 > 10) &&(arus2 > 10)){supply = "HYBRID";}
  if(arus2 < 10){supply = "PLN";}
  if(arus1 < 10){supply = "PLTS";}
}

void kirimData(){
  if(transferData < 1){
    String data = String(vrms1, 0) +"#" +String(vrms2, 0) +"#" +String(vrms3, 0) +"#";
    data += String(irms1, 2) +"#" +String(irms2, 2) +"#" +String(irms3, 2) +"#";
    data += String(P1, 2) +"#" +String(P2, 2) +"#" +String(P3, 2) +"#";
    data += supply +"#";
    data += I1 +"#" +I2 +"#" +I3 +"#";

    mySerial.print(data);
    Serial.println(String("Data: ") +data);
    transferData++;
  }
}

void periksaPermintaan(){
  char minta1[] = "123";
  char minta2[] = "123";
  String arrData[2];

  if(banyakPeriksaPermintaan < 1){
    String mintaData;

    while(mySerial.available() > 0){
        mintaData += char(mySerial.read());
    }
    
    mintaData.trim();
    if(mintaData != ""){
      if(mintaData.length() == 8){
        int index = 0;
        for(unsigned int i = 0; i <mintaData.length(); i++){
          char pemisah = '#';
          if(mintaData[i] != pemisah){
            arrData[index] += mintaData[i];
          }else{
            index++;
          }
        }
        
      }
    }
  int cek1 = strcmp(arrData[0].c_str(), minta1);
  int cek2 = strcmp(arrData[1].c_str(), minta2);
  if((cek1 == 0) ||(cek2 ==0)){
    Serial.println(String("Isi 1: ") +arrData[0] +"\tIsi 2: "+arrData[1]);
    kirimData();
  }
   
  banyakPeriksaPermintaan++;
  }
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);

  //Set Sensor ACS712
  ACS1.autoMidPoint();
  ACS2.autoMidPoint();
  ACS3.autoMidPoint();
  
  //Set Relay
  pinMode(relayPLTSPLN1, OUTPUT);
  pinMode(relayPLTSPLN2, OUTPUT);
  pinMode(relayInverter1, OUTPUT);
  pinMode(relayInverter2, OUTPUT);
  pinMode(relayIndikatorPLTS, OUTPUT);
  pinMode(relayIndikatorPLN, OUTPUT);
  
  //Set Nilai 0 Untuk Semua Parameter
  banyakPeriksaPermintaan = 0;
  panjangData = 0;
}

void loop() {
  waktu = millis();
  bacaTegangan();
  bacaArus();
  hitungDaya();
  setRelayInverter();
  setSupplyName();
  
  if(waktu - last > interval){
    last = waktu;
    periksaPermintaan();
    
    banyakPeriksaPermintaan = 0;
    transferData = 0;
  }
}
