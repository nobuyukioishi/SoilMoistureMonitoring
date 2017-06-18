#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

#define MCP3204_CS 5 // CSpinの出力先を指定

//ESP-WROOM-02でアナログ入力をするために必要
extern "C" {
#include "user_interface.h"
}

// Update these with values suitable for your network.

const char* ssid = "enter-your-ssid";
const char* password = "enter-your-pass";
const char* mqtt_server = "enter-mqtt-broker-ip";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {
  delay(10);

  digitalWrite(4, HIGH);
  delay(400);
  digitalWrite(4, LOW);
  delay(400);
  digitalWrite(4, HIGH);
  delay(400);
  digitalWrite(4, LOW);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

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

  digitalWrite(4, HIGH);
  delay(2000);
  digitalWrite(4, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("soilMoistureMonitoring", "How you doing?");
      // ... and resubscribe
      // for me, don't need to subscribe
      // client.subscribe("inTopic");
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
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883); // 1883 is a port No.
  client.setCallback(callback);

  Serial.begin(115200); //シリアル通信も通常のArduinoとは違う
  pinMode(4, OUTPUT); // use when you want use LED

  // ＳＰＩの初期化
  SPI.begin() ;                           // ＳＰＩを行う為の初期化
  SPI.setBitOrder(MSBFIRST) ;             // ビットオーダー
  SPI.setDataMode(SPI_MODE1) ;            // CLK極性 0(idle=LOW)　CLK位相 1(LOW > HIGH)
  SPI.setClockDivider(SPI_CLOCK_DIV8) ;   // SPI通信クロック(CLK)は2MHz

  delay(3000) ;                           // 3Sしたら開始
}



// TOUTピンからの入力値を取得
// 湿度センサからの値を読み取る
/* A/D 変換するからいらなくなった
int getToutValue(){
 int res = system_adc_read(); //ここでTOUTの値を取得
 return res;
}
*/
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  long now = millis();

  if (now - lastMsg > 1000*30 ) {
    // int val= getToutValue();
    int val0;

    // MCP3208のCH0からアナログ値を読み込む
    val0 = ADC_analogRead(MCP3204_CS, 0);
    Serial.print("val0: ");
    Serial.println(val0);
    lastMsg = now;

    snprintf (msg, 75, "%d", val0);
    Serial.print("soil moisture level: ");
    Serial.println(msg);
    client.publish("soilMoistureMonitoring", msg);
    // checking if it works
    digitalWrite(4, HIGH);
    delay(400);
    digitalWrite(4, LOW);
  }
}

// ADC_analogRead(ss,channel)   MCP3208からアナログ値を読み取る処理
//  ss      : SPIのSS(CS)ピン番号を指定する
//  channel : 読み取るチャンネルを指定する(0-3ch)
int ADC_analogRead(int ss,int channel)
{
     static char f ;
     int d1 , d2 ;

     // 指定されたSSピンを出力に設定する(但し最初コールの１回のみ)
     if (f != 1) {
          pinMode(ss,OUTPUT) ;
          digitalWrite(ss,HIGH) ;
          f = 1 ;
          delay(1) ;
     }
     // ADCから指定チャンネルのデータを読み出す
     digitalWrite(ss,LOW) ;              // SS(CS)ラインをLOWにする
     d1 = SPI.transfer( 0x06 | (channel >> 2) ) ;
     d1 = SPI.transfer( channel << 6 ) ;
     d2 = SPI.transfer(0x00) ;
     digitalWrite(ss,HIGH) ;             // SS(CS)ラインをHIGHにする

     return (d1 & 0x0F)*256 + d2 ;
}
