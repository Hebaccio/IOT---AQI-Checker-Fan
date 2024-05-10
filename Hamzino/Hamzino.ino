#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "HebaccioIOT"
#define WIFI_PASSWORD "HebaccioIOT"
#define API_KEY "AIzaSyC6fc_YTYAbCvYKj4QJQAjDiFtMC8srh1k"
#define DATABASE_URL "https://gas-sensor-iot-42b82-default-rtdb.europe-west1.firebasedatabase.app/"

#define AO_PIN A0
#define D1_PIN D1
#define D2_PIN D2
#define D3_PIN D3
#define GRN_PIN GRND

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
double gasValue = 0;
bool manuelno = false;


void setup() {
  // Initialize the Serial to communicate with the Serial Monitor.
  Serial.begin(115200);
  Serial.println("Warming up the MQ2 sensor");
  delay(10000);  // wait for the MQ2 to warm up

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to WiFi");

    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print("Connecting");
      for(int i = 0; i<=2; i++){
        delay(100);
        Serial.print(".");
      }
      delay(550);
    }

    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Sign up */
    if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("SignUp OK");
      signupOK = true;
    }
    else 
    {
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}
void loop(){
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) 
    {
      sendDataPrevMillis = millis();
      gasValue = analogRead(AO_PIN);
      delay(500);

      Serial.print("MQ2 sensor AO value: ");
      Serial.println(gasValue);
      delay(500);

      if(Firebase.RTDB.getBool(&fbdo, "/Sensor/Manuelno")) 
      {
        if (fbdo.dataType() == "boolean")
        {
          manuelno = fbdo.boolData();
          if(manuelno)
          {
             onoff();
          } 
          else
          {
            analogWrite(D3, 0);
             
            if (Firebase.RTDB.setDouble(&fbdo, "Sensor/Data", gasValue))
            {
              if(gasValue<=550)
              {
                func();
                analogWrite(D1, 120);
              }
              else if(gasValue>550 && gasValue<=600)
              {
                func();
                analogWrite(D2, 120);
              }
              else
              {
                func();
                analogWrite(D2, 220);
              }

              if (isnan(gasValue)) 
              {
                Serial.println(F("Failed to read from MQ2 sensor!"));
                return;
              }
            }
            else
            {
                Serial.println("FAILED! REASON: " + fbdo.errorReason());
            }
          }
        }
      }
    }
}
void func(){
  analogWrite(D1, 0);
  analogWrite(D2, 0);
  analogWrite(D3, 0);
}
void onoff(){
  analogWrite(D3, 200);
  delay(1000);
}
