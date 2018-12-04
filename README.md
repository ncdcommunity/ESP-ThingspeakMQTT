# SHT 25 MQTT  ThingSpeak Tutorial

It's an ESP8266 project to measure Temperature and Humidity using SHT31 and Adafruit Huzzah ESP8266. It provides us Real Time    Temperature and Humidity data and hourly analytics. The data is sent using MQTT thing speak API and later we are providing an email notification to the user whenever temperature reaches assigned threshold using IFTTT protocol

## Features 

  - Provides you real time analytics and stats using Thing Speak MQTT API 
  - An Email Notification is provided to the user at assigned time using IFTTT 
  - Task Scheduler are used to Schedule the task like fetching data from sensors, Publishing the sensor readings, Subscribing to MQTT       topic 
  - It uses I2C protocol to fetch the sensor reading which are more accurate, expandable and scalable
  - sleep mode when device is idle or no task callback is called.
  - effective task scheduling provide hastle free usage
  - A seperate webpage is hosted where user has to provide his user credentials to avoid flashing your device everytime are in reach of other wifi networks
  - SPIFFS is used to store our webpage to make our code readable and less clumsy

## Hardware Specification
  - [Adafruit esp8266 Huzzah board](https://www.adafruit.com/product/2471)
  - [Huzzah Board Shield](https://store.ncd.io/product/i2c-shield-for-adafruit-huzzah-esp8266-integrated-usb-and-i2c-port/)
  - [SHT25 Sensor module](https://store.ncd.io/product/sht25-humidity-and-temperature-sensor-%C2%B11-8rh-%C2%B10-2c-i2c-mini-module/)
  - [I2C cable](https://store.ncd.io/product/i%C2%B2c-cable/)
 

##Software Specification
 - Arduino IDE
 - Thing Speak MQTT API 

## How it works?

  -  We have scheduled two tasks refering to two different control operations
  - Task 1 is for reading the sensor value this task runs for 1 second till it reaches timeout of 10 secs.
  - When the Task1 reaches its time out We are connecting to local Wifi and MQTT broker. 
  - Now Task 2 is enabled and we are disabling Task 1 
  - Task 2 is for publishing the sensor data to Thing Speak MQTT broker this task runs for 20 second till it reaches timeout of 20 secs 
  - When the Task2 reaches its time out Task 1 is enabled again and Task2 is disabled. here again we are getting the updated value and       the process goes on   
  - when no callback is called or the device is idle it goes to Light Sleep thus saving power.
  - We are also using IFTTT protocol to provide user with email notifications on weather updates
 

## Usage

Before proceding to our mesh we need to save the user credentials. For this purpose we are hosting a web server at 192.169.1.4. Once the device starts it hosts a web server for 60 secs. The user should follow these steps.
    

 - Connect to the AP ESPuser, This is listed in your available wifi network list. Connect to this AP and enter password 24041990 
 - Once it gets connected, Go to your browser enter the the IP 192.168.1.4. 

    ![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture7.PNG "ESP8266 WebForm")
  
 - Enter the ssid and password of your local WiFi in the input fields and enter SUBMIT
 
 - These credential will be saved in EEPROM
 
    ![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture8.PNG "Local Wifi Connection")
 
 
 - After 60 sec Device will automaticaly disconnect from AP 
 
 - Next time when you turn on the device, The user doesn't have to follow this procedure, The device will automaticaly fetch the user      credentials from EEPROM and continue with getting the sensor readings from mesh and posting it to cloud


## Profile

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture1.PNG "Task1 - Reading values from SHT25")

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture2.PNG "Task2 - Publishing values to ThingSpeak")

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture3.PNG "ThingSpeak TempC graph")

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture4.PNG "ThingSpeak TempF graph")

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture6.PNG "ThingSpeak Humidity graph")

![alt text](https://github.com/vbshightime/ESP-ThingspeakMQTT/blob/master/Capture9.PNG "IFTTT email Notification")


## Limitations

- There are some issues with publishing the data using publish meathod for the large bulk of data. to resolve this issue we are using write() function  
- SPIFFS should be formated before uploading the new data to SPIFFS. 
- You must not use the delay() function. delay() hinders the background operation. Instead create delays using millis() only if it is necessary

## Credits

- [TaskScheduler](https://github.com/arkhipenko/TaskScheduler)
- [PubSubClient](https://pubsubclient.knolleary.net/)
