#include <Wire.h>
#include <GSM.h>
#include <PubSubClient.h>

const int MPU_addr=0x68;  // I2C address of the MPU-6050
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

// PIN Number
#define PINNUMBER ""

// APN data
#define GPRS_APN       "GPRS_APN" // replace your GPRS APN
#define GPRS_LOGIN     "login"    // replace with your GPRS login
#define GPRS_PASSWORD  "password" // replace with your GPRS password

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// initialize the library instance
GSMClient gsmClient;
GPRS gprs;
GSM gsmAccess;     // include a 'true' parameter for debug enabled
PubSubClient mqttClient(gsmClient);


// URL & port
char server[] = "broker.hivemq.com";
int port = 1883; // port 1883 is the default for MQTT

// timeout
const unsigned long __TIMEOUT__ = 10*1000;


void setup(){
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  // initialize serial communications
  Serial.begin(9600);
  Serial.println("Initializing...");
  
  // connection state
  boolean notConnected = true;

  // Start GSM shield
  // If your SIM has PIN, pass it as a parameter of begin() in quotes
  while(notConnected)
  {
    if((gsmAccess.begin(PINNUMBER)==GSM_READY) &
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD)==GPRS_READY))
      notConnected = false;
    else
    {
      Serial.println("Not connected");
      delay(1000);
    }
  }

  Serial.println("Connected to GPRS network");

  //Get IP.
  IPAddress LocalIP = gprs.getIPAddress();
  Serial.println("IP address=");
  Serial.println(LocalIP);

  mqttClient.setServer(server, port);
  mqttClient.setCallback(callback);
}


void loop(){
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true);  // request a total of 14 registers
  AcX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)     
  AcY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  GyX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
  GyY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)

  Tmp = Tmp/340.00+36.53; //equation for temperature in degrees C from datasheet
  
/*  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.print(AcZ);*/
  Serial.print(" Tmp = "); Serial.println(Tmp); 
 /* Serial.print(" | GyX = "); Serial.print(GyX);
  Serial.print(" | GyY = "); Serial.print(GyY);
  Serial.print(" | GyZ = "); Serial.println(GyZ);*/

  if (!mqttClient.connected()) {
  // Loop until we're reconnected
    while (!mqttClient.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (mqttClient.connect("arduinoClient")) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        mqttClient.publish("Versys","Hello World!");
        // ... and resubscribe
        mqttClient.subscribe("Opiframe");
      } else {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  } else {
    String data = " {\"AcX\":" + String(AcX) + ", " + "\"AcY\":" + String(AcY) + ", " + "\"AcZ\":" + String(AcZ) + "}\n " ;
    int length = data.length();
    char Ac[length];
    data.toCharArray(Ac,length+1);
    Serial.print(Ac);
    mqttClient.publish("Versys", Ac);
    
    data = " {\"GyX\":" + String(GyX) + ", " + "\"GyY\":" + String(GyY) + ", " + "\"GyZ\":" + String(GyZ) + "}\n " ;  
    length = data.length();
    char Gy[length];
    data.toCharArray(Gy,length+1);
    Serial.print(Gy);
    mqttClient.publish("Versys", Gy);
}

  mqttClient.loop();
  delay(250);
}
