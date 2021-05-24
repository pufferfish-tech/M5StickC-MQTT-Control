#include <M5StickC.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "AXP192.h"
#include "config.h"
#include "time.h"
#include <rom/rtc.h>

char* mqtt_topic = "iot2/m5stick";  //this is the MQTT topic to send the elapsed time with

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

// connect wifi
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;
const char* mqtt_server = MQTT_SERVER;

int brightness = 7;  //7 is lowest for ...reasons

//MQTT client
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "MQTT_Werkplaats";
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("hue/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
    M5.begin();
    Serial.begin(115200);
    
    // we need to know why we booted up - deep sleep or actual power on?
    Serial.println(" ");
    Serial.println("Hello!");
    Serial.println("I was woken up for reason number (1 is reset, 5 is deep sleep..):");
    Serial.println(rtc_get_reset_reason(0));
    if ( rtc_get_reset_reason(0) == SW_CPU_RESET || rtc_get_reset_reason(0) == POWERON_RESET ) {  //if we actually reset

            Serial.println("looks like an actual reset, so I'll initialise some stuff");
              //reset RTC to 0:00:00
              RTC_TimeTypeDef TimeStruct;
              TimeStruct.Hours   = 0;
              TimeStruct.Minutes = 0;
              TimeStruct.Seconds = 0;
              M5.Rtc.SetTime(&TimeStruct);

              //worth noting that if you achieve more than 24 hours it should roll around to zero again 
              //this can of course be addressed by setting the date as well as the time, or even simpler just have a "days elapsed" flag
        
   }

   M5.Lcd.setRotation(1);
   M5.Axp.EnableCoulombcounter();
   M5.Axp.ScreenBreath(brightness);  //screen brightness (values 7->12)
   M5.Axp.SetLDO2(false);
   setup_wifi();
   client.setServer(mqtt_server, 1883);
   delay(1000);
  
}



void loop() {
  String figuretodisplay = String(M5.Axp.GetBatVoltage());
  M5.Rtc.GetTime(&RTC_TimeStruct);
  String timestring = String(RTC_TimeStruct.Hours) +"h"+String(RTC_TimeStruct.Minutes)+"m"+String(RTC_TimeStruct.Seconds)+"s";
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
   
  Serial.print("sending mqtt message..");
  
  String mqqtstring = "battery voltage: " + figuretodisplay + " / " + timestring;
  int str_len = mqqtstring.length() + 1; 
  char mqqt_char_array[str_len];
  mqqtstring.toCharArray(mqqt_char_array, str_len);
  
  client.publish(mqtt_topic,mqqt_char_array);
  
  delay(1000); //give it time to send
  WiFi.disconnect();

  esp_sleep_enable_timer_wakeup(SLEEP_MIN(30));  // 300 = 5 min, times 6 = 30 min OR we could always use the defined functions from the axp.h file! 
  M5.Axp.SetSleep();  // REMEMBER this will turn OFF the esp cpu power supply unless you change the code in axp.cpp! 
  esp_deep_sleep_start();

}
