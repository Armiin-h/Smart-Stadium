#include <dht11.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ThingerESP8266.h> //THINGER.IO library
#include <PubSubClient.h>
#include <EEPROM.h>

#define DHT11PIN 4
#define BLYNK_PRINT Serial
#define user "Armin"
#define device_Id "device1"
#define device_credentials "oO58JVgWjUbzAt"
ThingerESP8266 thing(user, device_Id, device_credentials);
dht11 DHT11;
int motorPin = 3;
int RelayPin = 7;
// Motor connections
int enA = 11;
int in1 = 10;
int in2 = 9;
char auth[] = "KVOokp_JcNd-ywyUlxoHJBi13otyiCFG";
char ssid[] = "Galaxy S20 FE 5G1DF8";
char pass[] = "kqgz7070";
const char* mqtt_server = "35.156.19.218";
//broker.hivemq.com

WiFiClient espClient;
PubSubClient client(espClient);

BlynkTimer timer;
float t;
float h;
int tempchangeflag;
int tempchangesize;
char *roofstate; 
int roofflag ;
int fanflag ;
void setup_wifi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void reconnect() {
  Serial.println("In reconnect...");
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_server)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(500);
    }
  }
}

void setup()
{
  EEPROM.begin(512);
  Serial.begin(115200);
  
  roofflag=EEPROM.read(0);
  fanflag=EEPROM.read(1);
 
   //Set all the motor control pins to outputs
   pinMode(enA, OUTPUT);
   pinMode(in1, OUTPUT);
   pinMode(in2, OUTPUT);


  setup_wifi();
  client.setServer(mqtt_server,1883);
  
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(1000L, sendSensor);
  timer.setInterval(1000L, sendth);
  Blynk.virtualWrite(V10, 0);
  
  thing.add_wifi(ssid, pass); 
}


void loop()
{
  
  if (!client.connected()) {
    reconnect();
  }

  Blynk.run();
  timer.run();
}

void sendSensor()
{
  int chke = DHT11.read(DHT11PIN);
  Blynk.syncVirtual(V10);
  Blynk.syncVirtual(V9); 
   
  int temp;   
  if (tempchangeflag == 1)                          
  {
    temp = (int)DHT11.temperature +  tempchangesize;
  }
  else
  {
    temp = (int)DHT11.temperature;
  }
  
               
  
  h = (int)DHT11.humidity;
  t = temp;
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  Blynk.virtualWrite(V5, h);
  Blynk.virtualWrite(V6, t);
}

void sendth() {
  int chk = DHT11.read(DHT11PIN);
  
  Blynk.syncVirtual(V10);
  Blynk.syncVirtual(V9); 
   
  if (tempchangeflag == 1)                          
  {
    DHT11.temperature = (int)DHT11.temperature +  tempchangesize;
  }
  
  delay(1000);
  Blynk.virtualWrite(V7, "Roof:   Fan:");
  
  if (DHT11.temperature > 38) {
    Serial.println("roof is closing");
    Blynk.virtualWrite(V8, "roof is closing");
    
    //turn on motor close roof();
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

    //delay(10000);
    //turnoffmotor();
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);

    //turn on fan
    digitalWrite(RelayPin, LOW);
    
    
    
    Blynk.setProperty(V6, "color", "#D3435C");
    
    roofflag = 1;
    fanflag = 1;
    
    delay(13000);
    Blynk.virtualWrite(V8, "closed    ON");
    roofstate = "closed";
  }
  if (DHT11.temperature < 27 && roofflag == 1) {
    Serial.println("roof is opening");
    Blynk.virtualWrite(V8, "roof is opening");
    
    //turn on motor open roof();
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    //delay(10000);
    //turnoffmotor();
    digitalWrite(in1, 0);
    digitalWrite(in2, 0);

    //turn off fan
    digitalWrite(RelayPin, HIGH);

    Blynk.setProperty(V6, "color", "#fff9e8");
    

    roofflag = 0;
    fanflag = 0;
    
    delay(13000);
    Blynk.virtualWrite(V8, "open    OFF");
    roofstate = "closed";   
  }
 
  if (roofflag == 0){
    Blynk.virtualWrite(V8, "open    OFF");
    roofstate = "open";
  }
  
  if ((float)DHT11.temperature < 40){
  Blynk.setProperty(V6, "color", "#adb2ff");
  }

  String tempandhumstr;
  tempandhumstr = (String)DHT11.temperature + "," + (String)DHT11.humidity + "," + (String)roofflag + "," + (String)fanflag;
  Serial.println(tempandhumstr);
  
  const char *tempandhumstr1  = tempandhumstr.c_str();
  client.publish("Tempretureandhumidity", tempandhumstr1);

  
  thing.handle();
  thing["temphumreport"] >> [](pson &out){
  out["temperature"] = DHT11.temperature;
  out["humadity"] = DHT11.humidity;
  };
  thing.stream("temphumreport");
  thing.write_bucket("SmartStadiumBucket", "temphumreport");


  EEPROM.write(0,roofflag);
  EEPROM.write(1,fanflag);
  EEPROM.commit();
  delay(1000);
}


BLYNK_WRITE(V10) 
{   
  tempchangeflag = param.asInt(); // Get value as integer
}
BLYNK_WRITE(V9) 
{   
  tempchangesize = param.asInt(); // Get value as integer
}
