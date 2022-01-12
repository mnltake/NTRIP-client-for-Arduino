#include <Arduino.h>
#include <M5StickC.h>

#define TINY_GSM_MODEM_SIM7080
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>

#define SerialMon Serial
#define SerialAT Serial1
#define SerialGPS Serial2

#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

#include<base64.h>
#define PERIOD 30
#define Debug 1

// Your GPRS credentials, if any
const char apn[]  = "povo.jp";
const char gprsUser[] = "";
const char gprsPass[] = "";

char* host = "rtk2go.com";
int httpPort = 2104; //RTK2GO Japanese regional Caster port 2104 http://rtk2go.com/regionalcastertables/
char* mntpnt = "MOUNTPOINT";
char* user   = "sample@gmail.com";//RTK2GO recommended email as the user name. http://rtk2go.com/
char* passwd = "";

TinyGsm        modem(SerialAT);
TinyGsmClient client(modem);
HttpClient   http_c(client, host, httpPort);

bool ntrip_start_stream();
bool ntrip_reqSrcTbl(char* host,int &port);   //request MountPoints List serviced the NTRIP Caster 
bool ntrip_reqRaw(char* host,int &port,char* mntpnt,char* user,char* psw);      //request RAW data from Caster 
bool ntrip_reqRaw(char* host,int &port,char* mntpnt); //non user
int ntrip_readLine(char* buffer,int size);

void setup() {
  M5.begin();//default baud rate 115200
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  SerialMon.println("Wait...");
  M5.Lcd.setCursor(5,40);
  M5.Lcd.print("Wait...");
  
  // Set GSM module baud rate
  //SerialMon.begin(19200);                   //monitor  
  SerialAT.begin(115200, SERIAL_8N1, 33, 32); //to SIM7080G
  SerialGPS.begin(38400, SERIAL_8N1, 36, 26); //to F9P UART ***GPIO36 is rx only***
  SerialBT.begin("GPSBT");
  delay(100);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  //modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print(F("waitForNetwork()"));
  while (!modem.waitForNetwork()) SerialMon.print(".");
  SerialMon.println(F(" Ok."));

  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  modem.gprsConnect(apn, gprsUser, gprsPass);
  SerialMon.println(F(" done."));

  SerialMon.print(F("isNetworkConnected()"));
  while (!modem.isNetworkConnected()) SerialMon.print(".");
  SerialMon.println(F(" Ok."));

  SerialMon.print(F("My IP addr: "));
  IPAddress ipaddr = modem.localIP();
  SerialMon.println(ipaddr);
  M5.Lcd.setCursor(5,0);
  M5.Lcd.println(ipaddr);
  if (!ipaddr) {
    delay(5000);
    ESP.restart();
  }
}

void loop() {
  long timeout = millis();
  SerialMon.println("start_stream");
  M5.Lcd.setCursor(5,0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.print("start_stream");
  ntrip_start_stream();

  while(!http_c.available()){
    if (millis() - timeout > 6000) {
      SerialMon.println("grtRTCM timeout");
      M5.Lcd.setCursor(5,40);
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.print("time out");
      delay(1000);
      break;
    }
  }
  while(http_c.available()) {
    //send RTCM 
    char ch = http_c.read();        
    SerialGPS.print(ch);
    //SerialMon.print(ch);
    SerialMon.print(".");
    M5.Lcd.setCursor(5,40);
    M5.Lcd.print("running..");

    // send NMEA
    if (SerialGPS.available() > 0){
    SerialBT.write(SerialGPS.read());
    }
  }
}

bool ntrip_start_stream(){
  Serial.println("Requesting SourceTable.");
  if(ntrip_reqSrcTbl(host,httpPort)){
    char buffer[512];
    delay(5);
    while(http_c.available()){
      ntrip_readLine(buffer,sizeof(buffer));
      SerialMon.print(buffer); 
    }
  }else{
    SerialMon.println("SourceTable request error");
    return false;
  }
  SerialMon.print("Requesting SourceTable is OK\n");
  http_c.stop(); //Need to call "stop" function for next request.
  
  SerialMon.println("Requesting MountPoint's Raw data");
  if(!ntrip_reqRaw(host,httpPort,mntpnt,user,passwd)){
    return false;
  }
  SerialMon.println("Requesting MountPoint is OK");
  return true;
}

bool ntrip_reqSrcTbl(char* host,int &port)
{
  if(!http_c.connect(host,port)){
      Serial.print("Cannot connect to ");
      Serial.println(host);
      return false;
  }/*p = String("GET ") + String("/") + String(" HTTP/1.0\r\n");
  p = p + String("User-Agent: NTRIP Enbeded\r\n");*/
  http_c.get(
      " / HTTP/1.0\r\n"
      "User-Agent: NTRIPClient for Arduino v1.0\r\n"
      );
  unsigned long timeout = millis();
  while (http_c.available() == 0) {
     if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        http_c.stop();
        return false;
     }
     delay(10);
  }
  char buffer[50];
  ntrip_readLine(buffer,sizeof(buffer));
  if(strncmp((char*)buffer,"SOURCETABLE 200 OK",17))
  {
    Serial.print((char*)buffer);
    return false;
  }
  return true;
    
}
bool ntrip_reqRaw(char* host,int &port,char* mntpnt,char* user,char* psw)
{
    if(!http_c.connect(host,port))return false;
    String p="/";
    String auth="";
    Serial.println("Request NTRIP");
    
    p = p + mntpnt + String(" HTTP/1.0\r\n"
        "User-Agent: NTRIPClient for Arduino v1.0\r\n"
    );
    
    if (strlen(user)==0) {
        p = p + String(
            "Accept: */*\r\n"
            "Connection: close\r\n"
            );
    }
    else {
        auth = base64::encode(String(user) + String(":") + psw);
        #ifdef Debug
        Serial.println(String(user) + String(":") + psw);
        #endif

        p = p + String("Authorization: Basic ");
        p = p + auth;
        p = p + String("\r\n");
    }
    p = p + String("\r\n");
    http_c.get(p);
    #ifdef Debug
    Serial.println(p);
    #endif
    unsigned long timeout = millis();
    while (http_c.available() == 0) {
        if (millis() - timeout > 20000) {
            Serial.println("Client Timeout !");
            return false;
        }
        delay(10);
    }
    char buffer[50];
    ntrip_readLine(buffer,sizeof(buffer));
    if(strncmp((char*)buffer,"ICY 200 OK",10))
    {
      Serial.print((char*)buffer);
      return false;
    }
    return true;
}
bool ntrip_reqRaw(char* host,int &port,char* mntpnt)
{
    return ntrip_reqRaw(host,port,mntpnt,"","");
}
int ntrip_readLine(char* _buffer,int size)
{
  int len = 0;
  while(http_c.available()) {
    _buffer[len] = http_c.read();
    len++;
    if(_buffer[len-1] == '\n' || len >= size) break;
  }
  _buffer[len]='\0';

  return len;
}
