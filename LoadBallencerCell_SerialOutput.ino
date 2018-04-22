#include "HX711.h"
#include "soc/rtc.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h> 
#include <TimeLib.h>

HX711 scale;
WiFiClientSecure net;
MQTTClient client;

//  your WIFI network SSID (name) and Password
char ssid[] = <WIFI_SSID>;          
char pass[] = <WIFI_PASSWORD>;

unsigned long lastMillis = 0;

long lastMsg = 0;
char msg[50];
int value = 0;

const int mqttPort = 8883;
const char *awsEndPoint = <AWS_End_Point>;
const char *subscribeTopic = "inTopic";
const char *publishTopic = "<AWS_Topic>";



const char* rootCABuff= <Root_Certificate>;

const char* privateKeyBuff = <Private_Certificate>;   

const char* certificateBuff = <Certificate_Key>;  //to verify the client


void connect() {
  Serial.print("Checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("ESP32")) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nconnected!");
}
     
void setup() {
  Serial.begin(115200);
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_80M);
  WiFi.begin(ssid, pass);

  net.setCACert(rootCABuff);
  net.setCertificate(certificateBuff);
  net.setPrivateKey(privateKeyBuff);

  client.begin(awsEndPoint, 8883, net);

  connect();

  Serial.println("Initializing the scale");
  scale.begin(26, 25);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());			// print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));  	// print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);	// print the average of 5 readings from the ADC minus tare weight (not set) divided
						// by the SCALE parameter (not set yet)

  scale.set_scale(2280.f);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();				        // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));		// print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
						// by the SCALE parameter set with set_scale

  Serial.println("Readings:");
}

void loop() {

  if (!client.connected()) {
    connect();
  }
  client.loop();
//  delay(100);
  
  if (millis() - lastMillis > 2000) {   //2 seconds non blocking delay
    lastMillis = millis();
    Serial.print(scale.get_units(1), 1);
    Serial.print("\n");
    time_t t = now();
    snprintf (msg, 75, "{\"time\":\"%d%d%d%d%d%d%d\",\"weight\":\"%.2f\",\"partNo\":\"part0001\"}",year(t),month(t),day(t),hour(t),minute(t),second(t),millis(),scale.get_units(10));
    //snprintf (msg, 75, "{weight:%d grams}", scale.get_units(1));
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(publishTopic, msg);
  }


  
//  Serial.print("one reading: \t");
//  Serial.print(scale.get_units(1), 1);
//  Serial.print("\n");
//  Serial.print("\t| average:\n");
//  Serial.println(scale.get_units(10), 1);

  scale.power_down();			        // put the ADC in sleep mode
  delay(100);
  scale.power_up();
}
