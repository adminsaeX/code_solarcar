
/*
 * Library for modbus RTU
 */
 
#include <ModbusRtu.h>

// data array for modbus network sharing
uint16_t au16data[16];
uint8_t u8state;
uint8_t u8query;
Modbus master(0,0,0);
modbus_t telegram[16];
unsigned long u32wait;
int ID_inverter = 1;
/*
 Supported DEVIO NB-DEVKIT I Board 
    |  Do not use PIN   |
    |      16 TX        |
    |      17 RX        |
    |      4 EINT       |
    |   26 power key    |
    |     27 reset      |

    If you have any questions, please see more details at https://www.facebook.com/AISDEVIO
*/
#include "AIS_SIM7020E_API.h"
#include "ClosedCube_HDC1080.h"
ClosedCube_HDC1080 hdc1080;
String address    = "178.128.219.211";               //Your IPaddress or mqtt server url
String serverPort = "1883";                         //Your server port
String clientID   = "2020044306574";               //Your client id < 120 characters
String topic      = "solar_pump_car";               //Your topic     < 128 characters
String payload    = "";                           //Your payload   < 500 characters
String username   = "admin-saeiot";               //username for mqtt server, username <= 100 characters
String password   = "qwertysae2021";               //password for mqtt server, password <= 100 characters 
unsigned int subQoS       = 0;
unsigned int pubQoS       = 0;
unsigned int pubRetained  = 0;
unsigned int pubDuplicate = 0;


const long interval = 60000;           //time in millisecond 
unsigned long previousMillis = 0;
int cnt = 0;

AIS_SIM7020E_API nb;
void setup() {
  //Serial.begin(9600);
  // telegram 0: read status
  telegram[0].u8id = ID_inverter; 
  telegram[0].u8fct = 3; 
  telegram[0].u16RegAdd = 8448; 
  telegram[0].u16CoilsNo = 1; 
  telegram[0].au16reg = au16data; 
  // telegram 1: read fault
  telegram[1].u8id = ID_inverter; 
  telegram[1].u8fct = 3; 
  telegram[1].u16RegAdd = 8450; 
  telegram[1].u16CoilsNo = 1; 
  telegram[1].au16reg = au16data+1; 
  // telegram 2: read frquency
  telegram[2].u8id = ID_inverter; 
  telegram[2].u8fct = 3; 
  telegram[2].u16RegAdd = 12288; 
  telegram[2].u16CoilsNo = 1; 
  telegram[2].au16reg = au16data+2;
   // telegram 3: read Bus Voltage
  telegram[3].u8id = ID_inverter; 
  telegram[3].u8fct = 3; 
  telegram[3].u16RegAdd = 12290; 
  telegram[3].u16CoilsNo = 1; 
  telegram[3].au16reg = au16data+3;
  // telegram 4: read Output Voltage
  telegram[4].u8id = ID_inverter; 
  telegram[4].u8fct = 3; 
  telegram[4].u16RegAdd = 12291; 
  telegram[4].u16CoilsNo = 1; 
  telegram[4].au16reg = au16data+4;
  // telegram 5: read Current
  telegram[5].u8id = ID_inverter; 
  telegram[5].u8fct = 3; 
  telegram[5].u16RegAdd = 12292; 
  telegram[5].u16CoilsNo = 1; 
  telegram[5].au16reg = au16data+5;
  // telegram 6: read Speed
  telegram[6].u8id = ID_inverter; 
  telegram[6].u8fct = 3; 
  telegram[6].u16RegAdd = 12293; 
  telegram[6].u16CoilsNo = 1; 
  telegram[6].au16reg = au16data+6;
  // telegram 7: read identify VFD
  telegram[7].u8id = ID_inverter; 
  telegram[7].u8fct = 3; 
  telegram[7].u16RegAdd = 8451; 
  telegram[7].u16CoilsNo = 1; 
  telegram[7].au16reg = au16data+7;
  nb.begin();
  setupMQTT();
  nb.setCallback(callback);   
  previousMillis = millis(); 
  hdc1080.begin(0x40); 
      
  
  master.begin(9600);
  master.setTimeOut( 1000 ); // if there is no answer in 5000 ms, roll over
  u32wait = millis() + 1000;
  u8state = u8query = 0;   
}
void loop() {   
   
   
   nb.MQTTresponse();
   dateTime timeClock = nb.getClock();
   String Signal = nb.getSignal();
   String temperature=String(hdc1080.readTemperature());
   String humidity=String(hdc1080.readHumidity());

   unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
        
        
        
        connectStatus();
                 
        nb.publish(topic,payload);      
//      nb.publish(topic, payload, pubQoS, pubRetained, pubDuplicate);      //QoS = 0, 1, or 2, retained = 0 or 1, dup = 0 or 1
        previousMillis = currentMillis;  
  }

   
   
    
   
  switch( u8state ) {
  case 0: 
    if (millis() > u32wait) u8state++; // wait state
    break;
  case 1: 
    master.query( telegram[u8query] ); // send query (only once)
    u8state++;
  u8query++;
  if (u8query > 9) u8query = 0;
    break;
  case 2:
    master.poll(); // check incoming messages
    if (master.getState() == COM_IDLE) {
      u8state = 0;
      u32wait = millis() + 100; 
    }
    break;
  }
  payload = "{\"client_ID\":\""+clientID+"\","
                  +"\"client_ig\":\""+Signal+"\","
                  +"\"identify_VFD\":\""+au16data[7]+"\","
                  +"\"pump_status\":\""+au16data[0]+"\","
                  +"\"pump_fault\":\""+au16data[1]+"\","
                  +"\"pump_freq\":\""+au16data[2]+"\","
                  +"\"pump_V\":\""+au16data[4]+"\","
                  +"\"pump_speed\":\""+au16data[6]+"\","
                  +"\"dcVolt\":\""+au16data[3]+"\","
                  +"\"dcCurrent\":\""+au16data[5]+"\","
                  +"\"temp\":\""+temperature+"\","
                  +"\"hum\":\""+humidity+"\","
                  +"\"date\":\""+timeClock.date+"\","
                  +"\"time\":\""+timeClock.time+"\"}";
                  
  
}
//=========== MQTT Function ================
void setupMQTT(){
  if(!nb.connectMQTT(address,serverPort,clientID,username,password)){ 
     Serial.println("\nconnectMQTT");
  }
    nb.subscribe(topic,subQoS);
//  nb.unsubscribe(topic); 
}
void connectStatus(){
    if(!nb.MQTTstatus()){
        if(!nb.NBstatus()){
           Serial.println("reconnectNB ");
           nb.begin();
        }
       Serial.println("reconnectMQ ");
       setupMQTT();
    }   
}
void callback(String &topic,String &payload, String &QoS,String &retained){
  Serial.println("-------------------------------");
  Serial.println("# Message from Topic \""+topic+"\" : "+nb.toString(payload));
  Serial.println("# QoS = "+QoS);
  if(retained.indexOf(F("1"))!=-1){
    Serial.println("# Retained = "+retained);
  }
}
