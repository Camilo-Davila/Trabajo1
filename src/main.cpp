#include <WiFi.h>
#include <TFT_eSPI.h>
#include <PubSubClient.h>

#include <Arduino.h>
#include <Wire.h>

#define REPORTING_PERIOD_MS 1000

#define BUTTON_LEFT 0
#define RELAY 33
#include <string.h>

#include "ACS712.h"
ACS712 sensor(ACS712_20A, 32);

// Update these with values suitable for your network.

const char *ssid = "UPBWiFi";              // WiFi name
const char *password = "";                 // WiFi password
const char *mqtt_server = "34.201.250.63"; // Your assign IP

TFT_eSPI tft = TFT_eSPI();

char IString[8];

char PString[8];

char RMSString[8];

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

bool toggle = false;
char toggleString[25];

void isr()
{
  if (toggle)
  {
    Serial.println("on");
  }
  else
  {
    Serial.println("off");
  }
  toggle = !toggle;
}

void setup_wifi()
{

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("..Message ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == "esp32/led1")
  {
    if (messageTemp == "1")
    {
      Serial.println("relay on");
      digitalWrite(RELAY, LOW); // El Relé enciende en bajo
      tft.fillCircle(200, 40, 20, TFT_GREEN);
    }
    else if (messageTemp == "0")
    {
      Serial.println("relay off");
      digitalWrite(RELAY, HIGH);
      tft.fillCircle(200, 40, 20, TFT_DARKGREY);
    }
  }
  /*
    if (String(topic) == "esp32/led2")
    {
      if (messageTemp == "1")
      {
        Serial.println("led2 on");
        tft.fillCircle(200, 90, 20, TFT_YELLOW);
      }
      else if (messageTemp == "0")
      {
        Serial.println("led2 off");
        tft.fillCircle(200, 90, 20, TFT_DARKGREY);
      }
    }
    */
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP32Client22"))
    {
      Serial.println("connected");

      // subscribe
      client.subscribe("esp32/led1");
      // client.subscribe("esp32/led2");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  attachInterrupt(BUTTON_LEFT, isr, RISING);

  Serial.println("Calibrating... Ensure that no current flows through the sensor at this moment");
  sensor.calibrate();
  Serial.println("Done!");

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    // tft.fillScreen(TFT_BLACK);
    float U = 110.0;

    // To measure current we need to know the frequency of current
    // By default 50Hz is used, but you can specify desired frequency
    // as first argument to getCurrentAC() method, if necessary
    float Irms = (sensor.getCurrentAC() - 0.7) / 10;
    if (Irms <= 0)
    {
      Irms = 0;
    }

    float I = Irms / 0.707; // Intensidad RMS = Ipico/(2^1/2)
    float P = Irms * U;     // P=IV watts
    P = P * (0.5 * 1 / 3600);
    /*
    Serial.print("I: ");
    Serial.print(I, 3);
    Serial.print("A , Irms: ");
    Serial.print(Irms, 3);
    Serial.print("A, Potencia: ");
    Serial.print(P, 3);
    Serial.println("W");
    */
    delay(500);

    // To calculate the power we need voltage multiplied by current
    // float P = U * I;

    //********************************Falta calcular vatios/hora**********************************************

    // Serial.println(String("I = ") + I + " A");
    // Serial.println(String("P = ") + P + " Watts");

    dtostrf(I, 2, 3, IString); // variable, numero de digitos, numero decimales, arreglo donde guardarlo
    Serial.print("Corriente: ");
    Serial.println(IString);
    client.publish("esp32/corriente", IString);

    dtostrf(P, 2, 3, PString); // variable, numero de digitos, numero decimales, arreglo donde guardarlo
    Serial.print("Potencia: ");
    Serial.println(PString);
    client.publish("esp32/potencia", PString);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Potencia (W/h):", 5, 0, 2);
    tft.drawString(PString, 10, 20, 6);

    dtostrf(Irms, 2, 3, RMSString); // variable, numero de digitos, numero decimales, arreglo donde guardarlo
    Serial.print("Irms: ");
    Serial.println(RMSString);
    client.publish("esp32/RMS", RMSString);
    tft.setTextColor(TFT_SKYBLUE, TFT_BLACK);
    tft.drawString("Corriente RMS (A):", 5, 70, 2);
    tft.drawString(IString, 20, 90, 6);

    /*
    sprintf(toggleString, "%d", toggle);
    Serial.print("Toggle: ");
    Serial.println(toggleString);
    client.publish("esp32/sw", toggleString);
    */
  }
}
