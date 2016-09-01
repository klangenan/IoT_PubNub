#include <DallasTemperature.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <DHT.h>

#define ONE_WIRE_BUS		D3	
#define DHT_PIN			D4
#define DHT_TYPE		DHT11

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallas(&oneWire);
DHT dht(DHT_PIN, DHT_TYPE);

//--WiFi [Station Mode]
const char *ssid = "******";		// change with your Access Point SSID
const char *password = "******";	// change with your Access Point Password
WiFiClient client;
const int httpPort   = 80;
//--

//--PubNub
const char* pnubHost    = "pubsub.pubnub.com";
const char* pnubPubKey  = "*******";		// change with your PubKey
const char* pnubSubKey  = "*******";		// change with your SubKey
const char* pnubChannel = "iot_pubnub";
//--

unsigned long prevMillis = 0;
long interval = 5000;

float temperature;
float humidity;


void setup() {
  Serial.begin(115200);

  dallas.begin();
  dht.begin();
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }

}

void loop() {
  
  unsigned long currMillis = millis();
  if((unsigned long) (currMillis - prevMillis) >= interval){
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

	// dallas -> request temperature
	dallas.requestTemperatures();
	float tempDallas = dallas.getTempCByIndex(0);

    if(!isnan(tempDallas) || !isnan(temperature) || !isnan(humidity)) {
		Serial.println("Dallas Temperature : " + (String)tempDallas);
		Serial.println("Temperature : " + (String)temperature);
		Serial.println("Humidity : " + (String)humidity);

	  
		if (!client.connect(pnubHost, httpPort)) {
			Serial.println("PubNub connection failed !");
			return;
		}

		//DATA FORMAT : http://pubsub.pubnub.com /publish/pub-key/sub-key/signature/channel/callback/message      
		String url = "/publish/";
		url += pnubPubKey;
		url += "/";
		url += pnubSubKey;
		url += "/0/";
		url += pnubChannel;
		url += "/0/";
		url += "{\"temp\":" + (String) tempDallas + ",\"humi\":" + (String) humidity + "}";
		Serial.println(url);

	  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
		  "Host: " + pnubHost + "\r\n" +
		  "Connection: close\r\n\r\n");
      
      delay(10);

      while(client.available()){
        String line = client.readStringUntil('\r');
        Serial.print(line);
      }
      
      client.stop();
      Serial.println();
      Serial.println("PubNub connectioin closed !");
      
    }
    prevMillis = currMillis;
  }

}
