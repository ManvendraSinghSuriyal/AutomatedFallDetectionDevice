  #include <Wire.h>
#include <ESP8266WiFi.h>


const int addrss=0x68;  // this is the I2C Address of  MPU-6050 Sensor
int16_t AccX,AccY,AccZ,temp,GyroX,GyroY,GyroZ;
float AX=0, AY=0, AZ=0, GX=0, GY=0, GZ=0;
boolean fallStatus = false; //it stores if a fall has occurred 
boolean trgr1=false; //it store if first trigger (lower threshold) has occurred
boolean trgr2=false; //it store if second trigger (upper threshold) has occurred
boolean trgr3=false; //it store if third trigger (orientation change) has occurred
byte trgr1Count=0; //it store the counts past since trigger 1 was set true
byte trgr2Count=0; //it store the counts past since trigger 2 was set true
byte trgr3Count=0; //it store the counts past since trigger 3 was set true
int AnglChange=0;


// WiFi Credentials
const char *SSID =  "Redmi5";     //  WiFi Name
const char *PASS =  "12345678"; // WiFi Password
void sendEvent(const char *event);


const char *Host = "maker.ifttt.com";
const char *PrivateKey = "hUAAAz0AVvc6-NW1UmqWXXv6VQWmpiGFxx3sV5rnaM9";

// implementing setup()
void setup(){
 Serial.begin(115200);
 Wire.begin();
 Wire.beginTransmission(addrss);
 Wire.write(0x6B);  
 Wire.write(0);     // sets to zero (wakes up the MPU-6050)
 Wire.endTransmission(true);
 Serial.println("Wrote to IMU");
  Serial.println("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");   // it  print "..." till it not connected
  }

  Serial.println("");
  Serial.println("WiFi connected"); // print "WIFI connected when it gets Connected"
 }


 // implementing loop()
void loop(){
 readMPU();

// Measuring Orientation
 AX = (AccX-2050)/16384.00;
 AY = (AccY-77)/16384.00;
 AZ = (AccZ-1947)/16384.00;
 GX = (GyroX+270)/131.07;
 GY = (GyroY-351)/131.07;
 GZ = (GyroZ+136)/131.07;

 // calculating Amplitute vactor for 3 axis
 float rawAplitude = pow(pow(AX,2)+pow(AY,2)+pow(AZ,2),0.5);
 int amplitude = rawAplitude * 10;  // Mulitiplied by 10 bcz values are between 0 to 1
 Serial.println(amplitude);


 if (amplitude<=2 && trgr2==false){ //if AM breaks lower threshold (0.4g)
   trgr1=true;
   Serial.println("TRIGGER 1 ACTIVATED!!");
   }
 if (trgr1==true){
   trgr1Count++;
   if (amplitude>=12){ //if AM breaks upper threshold (3g)
     trgr2=true;
     Serial.println("TRIGGER2 is Activated!!");
     trgr1=false; trgr1Count=0;
     }
 }
 if (trgr2==true){
   trgr2Count++;
   AnglChange = pow(pow(GX,2)+pow(GY,2)+pow(GZ,2),0.5); Serial.println(AnglChange);
   if (AnglChange>=30 && AnglChange<=400){ //if orientation changes by between 80-100 degrees
     trgr3=true; trgr2=false; trgr2Count=0;
     Serial.println(AnglChange);
     Serial.println("TRIGGER3 is Activated!!");
       }
   }


 if (trgr3==true){
    trgr3Count++;
    if (trgr3Count>=10){ 
       AnglChange = pow(pow(GX,2)+pow(GY,2)+pow(GZ,2),0.5);
       //delay(10);
       Serial.println(AnglChange); 
       if ((AnglChange>=0) && (AnglChange<=10)){ //if orientation changes remains between 0-10 degrees
           fallStatus=true; trgr3=false; trgr3Count=0;
           Serial.println(AnglChange);
             }
       else{ //user regained normal orientation
          trgr3=false; trgr3Count=0;
          Serial.println("TRIGGER3 is DeActivated!!");
       }
     }
  }

 if (fallStatus==true){ // Event of  fall Detection
   Serial.println("A FALL is DETECTED!!");
   sendEvent("fall_detect"); 
   fallStatus=false;
   }
 if (trgr2Count>=6){ // We allows 0.5seconds for orientation change
   trgr2=false; trgr2Count=0;
   Serial.println("TRIGGER2 DeActivated!!");
   }
 if (trgr1Count>=6){ //We allows 0.5seconds for AM to break the upper threshold value

   trgr1=false; trgr1Count=0;
   Serial.println("TRIGGER2 DeActivated!!");
   }
  delay(100);
   }
void readMPU(){
 Wire.beginTransmission(addrss);
 Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
 Wire.endTransmission(false);
 Wire.requestFrom(addrss,14,true);  // request a total of 14 registers
 AccX=Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)    
 AccY=Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
 AccZ=Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
 temp=Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
 GyroX=Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
 GyroY=Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
 GyroZ=Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
 }
void sendEvent(const char *event)
{
  Serial.print("Connecting to "); 
  Serial.println(Host);
    // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int PORT = 80;
  if (!client.connect(Host, PORT)) {
    Serial.println("Connection failed");
    return;
  }
    // We now create a URI for the request
  String URL = "/trigger/";
  URL += event;
  URL += "/with/key/";
  URL += PrivateKey;
  Serial.print("Requesting  to URL: ");
  Serial.println(URL);
  // This will send the request to the server
  client.print(String("GET ") + URL + " HTTP/1.1\r\n" +
               "Host: " + Host + "\r\n" + 
               "Connection: close\r\n\r\n");
  while(client.connected())
  {
    if(client.available())
    {
      String line = client.readStringUntil('\r');
      Serial.print(line);
    } else {
      // No data yet, wait a bit
      delay(50);
    };
  }
  Serial.println();
  Serial.println("closing the connection");
  client.stop();
}
