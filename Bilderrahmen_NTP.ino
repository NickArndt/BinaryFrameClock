#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Timezone.h>                       //https://github.com/JChristensen/Timezone
#include <WiFiManager.h>                    //https://github.com/tzapu/WiFiManager
#include <FastLED.h>                        //https://github.com/FastLED/FastLED

#define NUM_LEDS 18                         //Number of LEDs (used to create the array)
#define DATA_PIN D3                         //Data-Pin on the Arduino
#define HourColor Red                         //Color to show for the Hour-Bits
#define MinuteColor Green                         //Color to show for the Minute-Bits
#define SecondColor Blue                         //Color to show for the Second-Bits
#define Neutral Black                         //Color to show for the Unused-Bits

int h, m, s, w, mo, ye, d;                        //Variablen für die lokale Zeit
unsigned int localPort = random(1024, 65535);                       //Zufälliger lokaler Port zum Abhören von UDP-Paketen
const char* ntpServerName = "de.pool.ntp.org";                        //Der verwendete DE-Pool

WiFiUDP udp;                        //Die verwendete UDP Instanz zum versenden von UDP-Pakten

CRGB leds[NUM_LEDS];                         //Create the LED-array

TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};                       //Regel für Central European Time (Berlin, Paris)
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};                         //Regel für Central European Time (Berlin, Paris)
Timezone CE(CEST, CET);

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);                         //Fill the LED-array
  WiFiManager wifiManager;                         //Konfiguration des WLANS mit WiFiManager
  wifiManager.autoConnect("Bilderrahmen"); 
  udp.begin(localPort);                       //Starten der UDP-Abfrage
  Serial.print("lokaler port: ");
  Serial.println(udp.localPort());
  setSyncProvider(gettime);
  setSyncInterval(3600);                       //Anzahl der Sekunden zwischen dem erneuten Synchronisieren ein. 86400 = 1 Tag
}

void loop() {
    lokaleZeit();
    Serial.print ("Es ist ");
    Serial.print(h);
    Serial.print(":");
    printDigits(m);
    Serial.print(":");
    printDigits(s);
    Serial.print(" Uhr am ");
    printDigits(d);
    Serial.print(".");
    printDigits(mo);
    Serial.print(".");
    Serial.println(ye);

    displayHour(h);                       //Show the hour in binary format
    displayMinute(m);                       //Show the minute in binary format
    displaySecond(s);                       //Show the second in binary format

    delay(1000);                       //Wait some msec

}

void lokaleZeit()                       //Wandelt die Zeit des NTP-Zeit mit Hilfe der oben definierten Regel in die lokale Zeit um
{
  time_t tT = now();
  time_t tTlocal = CE.toLocal(tT);
  h = hour(tTlocal);
  m = minute(tTlocal);
  s = second(tTlocal); 
  w = weekday(tTlocal);
  d = day(tTlocal);
  mo = month(tTlocal);
  ye = year(tTlocal);
}

void printDigits(int digits) //Setzt vor jeden einstelligen Wert eine 0
{
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

// Function found here: http://www.multiwingspan.co.uk/arduino.php?page=led5
void displayHour(byte Stunde)
{
for (int h = 0; h <= 5; h++) // count from most right Bit (0) to 6th Bit (5)
{
//Serial.print(bitRead(Stunde, h)); //Gibt Stunden in Bits zum Testen aus
if (bitRead(Stunde, h) == 1) // Read the single Bits from right to left and if there is a !1!, bring the LED up
{
leds[17 - h] = CRGB::HourColor; // show the color defined at the top of this sketch
}
else
{
leds[17 - h] = CRGB::Neutral; // if the LED is not used, take it down or to a different color. See top of the sketch
}
}
FastLED.show(); // Show the LEDs
}
 
void displayMinute(byte Minute)
{
for (int m = 0; m <= 5; m++)
{
if (bitRead(Minute, m) == 1)
{
leds[6 + m] = CRGB::MinuteColor; // Start at the 1st LED for the minutes (6) and add the Minute-Bit
}
else
{
leds[6 + m] = CRGB::Neutral;
}
}
FastLED.show();
}
 
void displaySecond(byte Sekunde)
{
for (int s = 0; s <= 5; s++)
{
if (bitRead(Sekunde, s) == 1)
{
leds[5 - s] = CRGB::SecondColor; // Start at the last LED for the seconds (5) and subtract the Seconds-Bits
}
else
{
leds[5 - s] = CRGB::Neutral;
}
}
FastLED.show();
}


/*----- Ab hier beginnt der NTP-Code ----*/

const long NTP_PACKET_SIZE = 48; //NTP-Zeit in den ersten 48 Bytes der Nachricht
byte packetBuffer[ NTP_PACKET_SIZE]; //Puffer für eingehende und ausgehende Pakete

time_t gettime() { //Ruft aktuelle NTP-Zeit (Sekunden seit 01.01.1900) ab und gibt die UNIX-Zeit (Sekunfen seit 01.01.1970) zurück
  IPAddress timeServerIP;
  WiFi.hostByName(ntpServerName, timeServerIP);// einen zufälligen Server aus dem Pool holen
  sendNTPpacket(timeServerIP); // sendet eine NTP-Paket an den Server
  delay(1000);// wait to see if a reply is available
  int size = udp.parsePacket();
  if (size >= NTP_PACKET_SIZE) {
    udp.read(packetBuffer, NTP_PACKET_SIZE); // Paket in den Puffer einlesen
    unsigned long secsSince1900;   // this is NTP time (seconds since Jan 1 1900)
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24; // vier Bytes ab Position 40 in eine lange Zahl umwandeln
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL; // Gibt die UNIX-Zeit zurück indem von der NTP-Zeit 70 Jahre (2208988800UL) abgezogen werden
  }
  return 0; // gibt 0 zurück wenn die Zeit nicht abgerufen werden konnte
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  memset(packetBuffer, 0, NTP_PACKET_SIZE);// set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // Nachdem alle NTP-Felder mit Werten versehen wurden, kann ein Paket gesendet werden das einen Zeitstempel anfordert:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
