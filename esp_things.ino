//header iles for scheduling tasks
#define _TASK_TIMEOUT
#include <TaskScheduler.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
//for SPIFFS
#include "FS.h"

#define Addr 0x40

//--------- AP config------------//
IPAddress ap_local_IP(192,168,1,4);
IPAddress ap_gateway(192,168,1,254);
IPAddress ap_subnet(255,255,255,0);

//--------- ThingSpeak MQTT parameters------------//
const char* mqtt_server = "mqtt.thingspeak.com";
char mqttUserName[] = "MQTT User Name";
char mqttPass[] = " MQTT API Key";

//--------- ThingSpeak Channel parameters------------//
const char writeAPIKey[] = "Your Write API Key";
static long channelID = 627641;
const char readAPIKey[] = "Your Read API Key";

WiFiClient wifiClient;
 

//--------- AP parameters------------//

const char *ssidAP = "ESPUser";
const char *passAP = "24041990";

//--------- Temp parameters------------//
 volatile float tempC;
 volatile float tempF;
 volatile float humid;

//--------- AP Timer------------//
unsigned long APTimer = 0;
unsigned long APInterval = 30000;

//--------- Task Timer------------//
unsigned long taskI2CTimer = 0;
unsigned long taskWiFiTimer = 0;

//--------- WiFi Timer------------//
unsigned long WiFiTimer = 0;
unsigned long WiFiInterval = 2000;

//--------- mqtt Timer------------//
unsigned long mqttTimer = 0;
unsigned long mqttInterval = 2000;


ESP8266WebServer server(80);

//class object for publishing and subscribing to mqtt broker
PubSubClient mqttCli;

//Class objct for Task Scheduler
Scheduler ts;

//---------prototype for task callback------------//
void taskI2CCallback();
void taskI2CDisable();
void taskWiFiCallback();
void taskWiFiDisable();


//---------Tasks------------//
Task tI2C(2 * TASK_SECOND, TASK_FOREVER, &taskI2CCallback, &ts, false, NULL, &taskI2CDisable);
Task tWiFi(20* TASK_SECOND, TASK_FOREVER, &taskWiFiCallback, &ts, false, NULL, &taskWiFiDisable);


//---------Setup function------------//
void setup() {
  // put your setup code here, to run once:

Serial.begin(115200);
EEPROM.begin(512);
SPIFFS.begin(); 

while(!Serial);

Serial.print("Configuring access point...");
WiFi.softAPConfig(ap_local_IP,ap_gateway,ap_subnet);
Serial.print("Setting up User Credentials");
WiFi.softAP(ssidAP,passAP);
server.on("/", handleRoot);
server.onNotFound(onHandleNotFound);
server.begin();
APTimer = millis();

while(millis()-APTimer<APInterval){
    server.handleClient();
  }

Wire.begin(2,14);

 mqttCli.setServer(mqtt_server,1883);
 mqttCli.setClient(wifiClient);
// mqttCli.setCallback(callback);
   
 tI2C.setTimeout(10 * TASK_SECOND);
 tWiFi.setTimeout(20 * TASK_SECOND);
 tI2C.enable();
}

//---------loop function------------//
void loop() {
  // tasks are executed using execute() method
  ts.execute();
}

//----------I2CCallback-----------//
void taskI2CCallback(){
  Serial.println("taskI2CStarted");
  Serial.print("timeout for this task: \t");
  Serial.println(tI2C.getTimeout());
    unsigned int data[2];
 
  // Start I2C transmission
  Wire.beginTransmission(Addr);
  // Send humidity measurement command, NO HOLD master
  Wire.write(0xF5);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(500);

  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);

  // Read 2 bytes of data
  // humidity msb, humidity lsb
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data
    float humidity = (((data[0] * 256.0 + data[1]) * 125.0) / 65536.0) - 6;
    
    // Output data to Serial Monitor
    Serial.print("Relative Humidity :");
    Serial.print(humidity);
    Serial.println(" %RH");
    humid = humidity;
  }

  // Start I2C transmission
  Wire.beginTransmission(Addr);
  // Send temperature measurement command, NO HOLD master
  Wire.write(0xF3);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(500);

  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);

  // Read 2 bytes of data
  // temp msb, temp lsb
  if(Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();

    // Convert the data
    float cTemp = (((data[0] * 256.0 + data[1]) * 175.72) / 65536.0) - 46.85;
    float fTemp = (cTemp * 1.8) + 32;

    tempC = cTemp;
    tempF = fTemp;
   
    // Output data to Serial Monitor
    Serial.print("Temperature in Celsius :");
    Serial.print(cTemp);
    Serial.println(" C");
    Serial.print("Temperature in Fahrenheit :");
    Serial.print(fTemp);
    Serial.println(" F");
  }
   
  
  }


//----------I2CDisable-----------//
void taskI2CDisable(){
  unsigned long taskTime = millis() - taskI2CTimer;
  Serial.println(taskTime/1000);
  taskI2CTimer = millis();
  if(tI2C.timedOut()){
        Serial.println("//taskI2C disabled");
        Serial.println("call taskAP");
        reconnectWiFi();
        reconnectMQTT();
        tI2C.setCallback(&taskWiFiCallback);
        tWiFi.enable();
        tI2C.disable();
    }
  }
  
//----------WiFiCallback-----------//
void taskWiFiCallback(){
  Serial.println("taskWiFiCallbackStarted");
  Serial.print("timeout for this task: \t");
  Serial.println(tWiFi.getTimeout());
      if(!mqttCli.connected()){
          Serial.println("Client not connected");
          reconnectMQTT();
        } 
        String topicString ="channels/"+String(channelID)+"/publish/"+String(writeAPIKey);
       int topicLength = topicString.length()+1;
       char topicBuffer[topicLength];
       topicString.toCharArray(topicBuffer,topicLength+1);
       Serial.println(topicBuffer);
       String dataString  = String("field1="+ String(tempC,1) + "&field2=" + String(tempF,1) + "&field3=" + String(humid,1));
       int dataLength = dataString.length()+1;
       byte dataBuffer[dataLength];
       dataString.getBytes(dataBuffer,dataLength);
       
       mqttCli.beginPublish(topicBuffer,dataLength,false);

       Serial.println(mqttCli.write(dataBuffer,dataLength) ? "published" : "published failed");
      
       mqttCli.endPublish();
       
       //mqttCli.loop();
  } 
   
//----------WiFiDisable-----------//
void taskWiFiDisable(){
   unsigned long taskTime = millis() - taskWiFiTimer;
  Serial.println(taskTime/1000);
  taskWiFiTimer = millis();
  if(tWiFi.timedOut()){
    Serial.println("//taskWiFi disabled");
        Serial.println("call taskI2C");
        //enable I2C task again and call taskI2CCallback
        tWiFi.setCallback(&taskI2CCallback);
        tI2C.enable();
        //disables WiFi task
        tWiFi.disable();
    }
  } 
  
//****************************HANDLE ROOT***************************//

void handleRoot() {
   if (server.hasArg("ssid")&& server.hasArg("password") ) {//If all form fields contain data call handelSubmit()
    handleSubmit();
  }
  else {
      //Redisplay the form
     //read the file contained in spiffs
      File  file =SPIFFS.open("/webform.html", "r");
      server.streamFile(file,"text/html");
      //don't forget to close the file
      file.close();
   }
}

//**************************SUBMIT RESPONSE**************************//
void handleSubmit(){//dispaly values and write to memmory
  String response="<p>The ssid is ";
 response += server.arg("ssid");
 response +="<br>";
 response +="And the password is ";
 response +=server.arg("password");
 response +="</P><BR>";
 response +="<H2><a href=\"/\">go home</a></H2><br>";

 server.send(200, "text/html", response);
 //calling function that writes data to memory 
 ROMwrite(String(server.arg("ssid")),String(server.arg("password")));
}

//----------Handle Not Found-----------//  
 void onHandleNotFound(){
      String message = "File Not Found\n\n";
      message += "URI: ";
      message += server.uri();
      message += "\nMethod: ";
      message += (server.method() == HTTP_GET)?"GET":"POST";
      message += "\nArguments: ";
      message += server.args();
      message += "\n";
      server.send(404, "text/plain", message);
}


//----------Write to ROM-----------//
void ROMwrite(String s, String p){
 s+=";";
 write_EEPROM(s,0);
 p+=";";
 write_EEPROM(p,100);
 EEPROM.commit();   
}


void write_EEPROM(String x,int pos){
  for(int n=pos;n<x.length()+pos;n++){
  //write the ssid and password fetched from webpage to EEPROM
   EEPROM.write(n,x[n-pos]);
  }
}


void reconnectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  char ssidWiFi[30];//Stores the router name
  char passWiFi[30];//Stores the password

        String string_Ssid="";
        String string_Password="";
        string_Ssid= read_string(30,0); 
        string_Password = read_string(30,100); 
        Serial.println("ssid: "+ string_Ssid);
        Serial.println("Password: "+string_Password);
        string_Password.toCharArray(passWiFi,30);
        string_Ssid.toCharArray(ssidWiFi,30);
        Serial.println(ssidWiFi);
        Serial.println(passWiFi);
        
  delay(400);
  WiFi.begin(ssidWiFi,passWiFi);
  while (WiFi.status() != WL_CONNECTED)
  {
      delay(500);
      Serial.print(".");
  }
  Serial.print("Connected to:\t");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT(){
   Serial.println("setting up mqtt");
   while(!mqttCli.connected()){
     if(mqttCli.connect("ESP8266Client123456789")==true){
        Serial.println("connected");
        String subTopic = String("channels/"+ String(channelID) + "/subscribe/json/" + String(readAPIKey));
        int subTopicLength = subTopic.length()+1;
        char subTopicBuffer[subTopicLength];
        subTopic.toCharArray(subTopicBuffer,subTopicLength);
        
        String pubMessage = "status=MQTTPUBLISH";
        String pubTopic =String("channels/"+String(channelID)+"/publish/"+String(writeAPIKey));
        int pubTopicLength = pubTopic.length()+1;
        char pubTopicBuffer[pubTopicLength];
        pubTopic.toCharArray(pubTopicBuffer,pubTopicLength);
        //Publish to MQTT Broker 
        Serial.println(mqttCli.publish(pubTopicBuffer, pubMessage.c_str()) ? "Published" : "NotPublished");
        //Subscribe to MQTT Broker
        Serial.println(mqttCli.subscribe(subTopicBuffer) ? "Subscribed" : "Unsbscribed"); 
       }else{
           Serial.print("failed, rc=");
           Serial.println(mqttCli.state());
           delay(1000);
        }
    }
  }


String read_string(int l, int p){
  String temp;
  for (int n = p; n < l+p; ++n)
    {
   // read the saved password from EEPROM
     if(char(EEPROM.read(n))!=';'){
     
       temp += String(char(EEPROM.read(n)));
     }else n=l+p;
    }
  return temp;
}

//helper method to clear EEPROM
void ROMClear(){
   for(int i = 0;i<512; i++){
    EEPROM.write(i,0);
    }
    EEPROM.end();
  }
