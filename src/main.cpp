#include <Arduino.h>
#include <SPI.h>

#include <User_Setup.h>

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <stdio.h>
#include <Stream.h>
#include <ArduinoJson.h>

#define BLUE 0x001F
#define TEAL 0x0438
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define ORANGE 0xFC00
#define PINK 0xF81F
#define PURPLE 0x8010
#define GREY 0xC618
#define WHITE 0xFFFF
#define BLACK 0x0000

size_t width = 189;
size_t height = 280;

TFT_eSPI tft = TFT_eSPI(width, height);

const char *ssid = "GEHEIM";
const char *password = "GEHEIM";

class StringStream : public Stream
{
public:
  StringStream(String &s) : string(s), position(0) {}

  // Stream methods
  virtual int available() { return string.length() - position; }
  virtual int read() { return position < string.length() ? string[position++] : -1; }
  virtual int peek() { return position < string.length() ? string[position] : -1; }
  virtual void flush(){};
  // Print methods
  virtual size_t write(uint8_t c)
  {
    string += (char)c;
    return 1;
  };

private:
  String &string;
  unsigned int length;
  unsigned int position;
};

float ParseFloat(String input)
{
  if (input.indexOf("fl_") >= 0)
  {
    input.remove(0, 3);
    union
    {
      uint32_t i;
      float f;
    } data;

    char out[9];
    input += "\0";
    input.toCharArray(out, 9);
    data.i = strtoul(out, NULL, 16);

    /*
      Serial.print("Hex Val,raw : ");
      Serial.println(input);
      Serial.print("Hex val out: ");
      Serial.println(out);
      Serial.print("Float val: ");
      Serial.println(data.f);
  */
    return data.f;
  }
  else
    Serial.println(input.indexOf("fl_"));
  return -1;
}

void setup(void)
{
  tft.init();
  tft.setRotation(2);
  pinMode(4, OUTPUT);
  //digitalWrite(4,HIGH);
  Serial.begin(9600);

  tft.setCursor(50, 0, 4);

  WiFi.begin(ssid, password);

  tft.setTextColor(RED, BLACK);

  while (WiFi.status() != WL_CONNECTED)
  {

    int ct = 0;
    String contxt = "Connecting";
    while (ct < 10)
    {
      tft.drawString(contxt, 60, 50);
      contxt += ".";
      delay(50);
      ct++;
    }
  }

  tft.setTextColor(GREEN, BLACK);
  tft.fillScreen(BLACK);
  tft.drawString("Connected!", 60, 50);
  Serial.println("Connected to the WiFi network");
  delay(2000);
  tft.fillScreen(BLACK);
}
HTTPClient http;
//92
#define MAX_indx 113
int indx = 0;
float vals[MAX_indx + 1];

void loop()
{
  http.begin("http://SENEC.IP/lala.cgi");       //Specify the URL
  http.addHeader("Content-Type", "application/json"); //Specify content-type header

  String httpRequestData = "{\"ENERGY\": {\"GUI_HOUSE_POW\": \"\", \"GUI_BAT_DATA_FUEL_CHARGE\": \"\",\"GUI_GRID_POW\": \"\",}}";
  //curl http://192.168.178.25/lala.cgi -H 'Content-Type: application/json' -d '{"ENERGY":{"GUI_BAT_DATA_POWER":"","GUI_BAT_DATA_FUEL_CHARGE":""}}'

  int httpResponseCode = http.POST(httpRequestData);
  if (httpResponseCode > 0)
  {

    String payload = "";
    StringStream stream(payload);

    http.writeToStream(&stream);

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    JsonObject obj = doc.as<JsonObject>();
    JsonObject energy = obj["ENERGY"];
    float BattPercent = ParseFloat(energy["GUI_BAT_DATA_FUEL_CHARGE"]);
    float GridPower = ParseFloat(energy["GUI_GRID_POW"]);

    Serial.print("Battery: ");
    Serial.println(BattPercent);
    Serial.print("Grid Power: ");
    Serial.println(GridPower);
    if (indx < MAX_indx)
    {
      vals[indx] = GridPower;
      indx++;
    }
    else
    {
      for (size_t i = 0; i < MAX_indx; i++)
      {
        vals[i] = vals[i + 1]; //shift all one to the left
      }
      vals[MAX_indx] = GridPower;
      /*
      for (size_t i = 0; i <= MAX_indx; i++)
      {
        Serial.print(vals[i]);
        Serial.print(" ");
      }
      Serial.println();
      */
    }

    if (indx > MAX_indx)
    {
      indx = MAX_indx;
    }
    uint32_t primColor;
    if (GridPower > 0)
    {
      tft.setTextColor(BLUE, BLACK); // RED is blue so BLUE IS RED?
      primColor = BLUE;
    }
    else
    {
      tft.setTextColor(GREEN, BLACK);
      primColor = GREEN;
    }

    //Batt vis vars
    int x = 70;
    int y = 50;
    int w = 115;
    int h = 15;
    tft.fillScreen(BLACK);

    tft.setCursor(50, 0, 4);

    tft.drawString("Battery:", 60, 70);
    tft.drawString(String(BattPercent) + " %", 70, 95);
    //BATTERY
    tft.drawRect(x-5, y, w, h, WHITE);
    float width = BattPercent * (w) / 100.0;
    //BATTERY FILL
    tft.fillRect(x -4, y + 1, width, h - 2, primColor);
    tft.drawString("Power:", 60, 125);
    if(abs(GridPower)<1000.0)
      tft.drawString(String(GridPower) + " W", 70, 145);
    else
      tft.drawString(String(GridPower/1000.0) + " kW", 70, 145);
    
    //CHART
    //tft.drawRect(x, 180, w , 100 - 5, WHITE);
    tft.setCursor(50, 0, 2);
    tft.setTextColor(RED);
    tft.drawString("P kW", 60, 175);
    
    int offset = 240;
    float scale = 80; //trial and error
    //y-axis
    tft.drawLine(x,(-4100.0 / scale) + offset,x,(2100.0 / scale) + offset,WHITE);


    tft.drawString("2",58,(2000.0 / scale) + offset-10);
    tft.drawString("1",58,(1000.0 / scale) + offset-10);

    tft.drawString("0",58, offset-10);

    tft.drawString("1",58,(-1000.0 / scale) + offset-10);
    tft.drawString("2",58,(-2000.0 / scale) + offset-10);
    tft.drawString("3",58,(-3000.0 / scale) + offset-10);
    for (size_t i = 0; i < MAX_indx; i++)
    {
      primColor = GREEN;
      float val = (vals[i] / scale) + offset;

      if (val > offset)
        primColor = BLUE;
      if (val == offset)
        primColor = RED; //remember blue is red and red is blue
      //tft.drawPixel(i + x, val, primColor);
      tft.drawLine(i+x,offset,i+x,val,primColor);
      if (i < 8)
      {
        tft.drawPixel(i + x - 3, (0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (1000.0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (-1000.0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (2000.0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (-2000.0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (-3000.0 / scale) + offset, WHITE);
        tft.drawPixel(i + x - 3, (-4000.0 / scale) + offset, WHITE);
      }
    }
  }
  else
  {
    Serial.println("Error on HTTP request");
    tft.drawString("HTTP Error", 60, 70);
    tft.drawString(String(httpResponseCode), 60, 90);
  }
  http.end();
  delay(1000);
}
