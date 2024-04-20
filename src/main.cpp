#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <time.h>

// Function declarations
void printLocalTime();
void getNTP();
void WiFi_on();
void WiFi_off();
void sendGPRMC();
uint8_t calculateChecksum(const char *sentence);

#include "credentials.h"
/*
const char *ssid = "ssid";
const char *password = "password";
*/

const char *ntpServer = "pool.ntp.org";                                  // Enter your closer pool or pool.ntp.org
const char *TZ_INFO = " 	CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Enter your time zone (https://remotemonitoringsystems.ca/time-zone-abbreviations.php)

// GPS data
const double latitude = 50.0755;  // Latitude of Prague
const double longitude = 14.4378; // Longitude of Prague
const double altitude = 200;      // Altitude of Prague
const int satellites = 8;         // Number of satellites
const int hdop = 1;               // Horizontal dilution of precision
const int fix = 1;                // Fix quality: 0 = invalid, 1 = GPS fix, 2 = DGPS fix
const int speed = 0;              // Speed over the ground in knots
const int course = 0;             // Course over the ground in degrees
const int date = 0;               // Date of fix
const int variation = 0;          // Magnetic variation in degrees
const int variationDirection = 0; // Direction of magnetic variation
const int mode = 0;               // Mode indicator

struct tm timeinfo;

SoftwareSerial mySerial(D2, D1, false); // RX, TX

void setup()
{
  Serial.begin(115200);

  mySerial.begin(4800);

  // Connect to Wi-Fi
  WiFi_on();

  // Set timezone
  setenv("TZ", TZ_INFO, 1);

  // Init and get the time
  getNTP();

  delay(1000);
  printLocalTime();
}

void loop()
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000)
  {
    previousMillis = currentMillis;
    printLocalTime();
    sendGPRMC();
  }
}

// -------------------------------------------------------- FUNCTIONS --------------------------------------------------------

void getNTP()
{
  Serial.print("GetNTP ");
  int i = 0;
  do
  {
    i++;
    if (i > 40)
    {
      ESP.restart();
    }
    configTime(0, 0, ntpServer);
    delay(500);
  } while (!getLocalTime(&timeinfo));
  Serial.println("Ok");
}

void WiFi_on()
{
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (counter > 20)
      ESP.restart();
    Serial.print(".");
    counter++;
  }
  Serial.println();
  Serial.println("WiFi connected");
}

void WiFi_off()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected");
  Serial.flush();
}

void printLocalTime()
{
  time_t now;
  struct tm *timeinfo;

  time(&now);
  // timeinfo = gmtime(&now); // returns GMT time!
  timeinfo = localtime(&now);

  Serial.println(asctime(timeinfo));

  if (timeinfo->tm_isdst == 0)
  {
    Serial.println("DST=OFF");
  }
  else
  {
    Serial.println("DST=ON");
  }
}

void sendGPRMC()
{
  time_t now;
  struct tm *timeinfo;

  time(&now);
  timeinfo = localtime(&now);

  // Generate GPRMC sentence
  // This sentence represents the Recommended Minimum Specific GNSS Data
  // It contains information such as time, date, latitude, longitude, speed, and course
  char gprmcSentence[200];

  // Declare variables for time data
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;
  int second = timeinfo->tm_sec;
  int day = timeinfo->tm_mday;
  int month = timeinfo->tm_mon + 1;
  int year = (timeinfo->tm_year + 1900) % 100;

  // Debug print
  Serial.print("hour: ");
  Serial.println(hour);
  Serial.print("minute: ");
  Serial.println(minute);
  Serial.print("second: ");
  Serial.println(second);
  Serial.print("day: ");
  Serial.println(day);
  Serial.print("month: ");
  Serial.println(month);
  Serial.print("year: ");
  Serial.println(year);

  // Fill sentence with variables
  sprintf(gprmcSentence, "$GPRMC,%02d%02d%02d,A,%.4f,N,%.4f,E,%d,%d,%02d%02d%02d,%d,E*", hour, minute, second, latitude, longitude, speed, course, day, month, year, variation);
  uint8_t checksum = calculateChecksum(gprmcSentence);
  sprintf(gprmcSentence + strlen(gprmcSentence), "%02X", checksum);
  Serial.println(gprmcSentence);
  mySerial.println(gprmcSentence);
}

// Function to calculate the checksum of an NMEA sentence
uint8_t calculateChecksum(const char *sentence)
{
  uint8_t checksum = 0;
  bool start = false;

  // Iterate through the sentence
  for (int i = 0; sentence[i] != '\0'; i++)
  {
    // Check for the start of the sentence
    if (sentence[i] == '$')
    {
      start = true;
      continue;
    }

    // Check for the end of the sentence
    if (sentence[i] == '*')
    {
      break;
    }

    // Calculate the checksum
    if (start)
    {
      checksum ^= sentence[i];
    }
  }

  return checksum;
}
