#include "AZ3166WiFi.h"
#include "IoT_DevKit_HW.h"
#include "http_client.h"
#include "string.h"
#include <ArduinoJson.h>

static bool hasWifi;
//HTTPClient *httpClient;
static const char* baseURL="https://corona.lmao.ninja/v2/";
static int country=-1;
static int screen=-1;
static int init; // used to track the initialisation when reading data
static bool pressedA=false;
static bool pressedB=false;

typedef struct
{
    char name[20];
    int cases;
    int deaths;
    int recovered;
    
    int todayCases;
    int todayDeaths;
    
    int active;
    int critical;
    
    int casesPerOneMillion;
    int deathsPerOneMillion;
    int recoveredPerOneMillion;
    
    int activePerOneMillion;
    int criticalPerOneMillion;
    
    int tests;
    int testsPerOneMillion;
} COVID_Stats;

static COVID_Stats stats[20];
static int lastCountry=-1;
static StaticJsonDocument<900> doc;

static void myCallback(const char* recv, size_t length) {

  //Screen.print(1, "Received");
  //Screen.print(2, recv);
  Serial.print("Received: ");
  Serial.println(recv);
  
}

void InitWiFi()
{
  if (WiFi.begin() == WL_CONNECTED)
  {
    IPAddress ip = WiFi.localIP();
    Screen.print(1, ip.get_address());
    hasWifi = true;
  }
  else
  {
    Screen.print(1, "No Wi-Fi           ");
  }
}

void parse(const char* data) {

  char buff[100];

  Serial.print("Parsing: ");
  Serial.println(data);
  //Screen.print(1,data,true);
  //delay(1000);

  DeserializationError error = deserializeJson(doc, data);
  
  if(error) {
    Serial.println("Error parsing data");
    //Screen.print(1,"Parse error");
    //Screen.print(2,error.c_str());
  } else {
    Serial.println("Data parsed");
    //Screen.print(1,"Parse OK");
    lastCountry++;
    //sprintf(buff,"%d",lastCountry);
    //Screen.print(2,buff);

    //there is no country for World results
    if (doc.containsKey("country")) {
      strcpy(stats[lastCountry].name,doc["country"]);
    } else {
      strcpy(stats[lastCountry].name,"World");
    }
    Serial.println(stats[lastCountry].name);
    //Screen.print(3,doc["country"]);
    
    stats[lastCountry].cases=doc["cases"];
    stats[lastCountry].deaths=doc["deaths"];
    stats[lastCountry].recovered=doc["recovered"];
    
    stats[lastCountry].todayCases=doc["todayCases"];
    stats[lastCountry].todayDeaths=doc["todayDeaths"];
    
    stats[lastCountry].active=doc["active"];
    stats[lastCountry].critical=doc["critical"];
    
    stats[lastCountry].casesPerOneMillion=doc["casesPerOneMillion"];
    stats[lastCountry].deathsPerOneMillion=doc["deathsPerOneMillion"];
    stats[lastCountry].recoveredPerOneMillion=doc["recoveredPerOneMillion"];
    
    stats[lastCountry].activePerOneMillion=doc["activePerOneMillion"];
    stats[lastCountry].criticalPerOneMillion=doc["criticalPerOneMillion"];
    
    stats[lastCountry].tests=doc["tests"];
    stats[lastCountry].testsPerOneMillion=doc["testsPerOneMillion"];
  }
  //delay(1000); for debugging only
}

void http_get(const char* url)
{

  char buff[255];
  strcpy(buff,baseURL);
  strcat(buff,url);
  
  HTTPClient *httpClient = new HTTPClient(HTTP_GET, buff); //, myCallback);
  const Http_Response* result = httpClient->send();

  if (result == NULL)
  {
    //Screen.print(1, "Failed");

    Serial.print("Error Code: ");
    Serial.println(httpClient->get_error());
  }
  else
  {
    //Screen.print(1, "Send OK");
    //char buff[100];
    //sprintf(buff,"%d",result->status_code);
    //Screen.print(2, buff);
    //Screen.print(3, result->body,true);
    //Screen.print(3, result->headers,true);

    Serial.println("Body");
    Serial.println(result->body);
    parse(result->body);
  }

  delete httpClient;
}

void makeLine(char*buff, char*prompt, int num) {

// simple function to make sure that we don't lose characters in the 16 character display
  sprintf(buff,prompt,num);
  if (strlen(buff)>16) {
    strcpy(buff,"Overflow");
  }
}

void showStats() {
  char buff[100];
  strcpy(buff,stats[country].name);
  Screen.print(0, buff);
  switch (screen) {
    case 0:
      makeLine(buff,"Cases %d",stats[country].cases);
     Screen.print(1, buff);
      makeLine(buff,"Deaths %d",stats[country].deaths);
      Screen.print(2, buff);
      makeLine(buff,"Recover %d",stats[country].recovered);
      Screen.print(3, buff);
      break;
     case 1:
       Screen.print(1, "New Today:");
      makeLine(buff," Cases %d",stats[country].todayCases);
      Screen.print(2, buff);
      makeLine(buff," Deaths %d",stats[country].todayDeaths);
      Screen.print(3, buff);
      break;
     case 2:
      Screen.print(1, "Currently:");
      makeLine(buff," Active %d",stats[country].active);
      Screen.print(2, buff);
      makeLine(buff," Critical %d",stats[country].critical);
      Screen.print(3, buff);
      break;
     case 3:
      makeLine(buff,"Cases/m %d",stats[country].casesPerOneMillion);
     Screen.print(1, buff);
      makeLine(buff,"Deaths/m %d",stats[country].deathsPerOneMillion);
      Screen.print(2, buff);
      makeLine(buff,"Recover/m %d",stats[country].recoveredPerOneMillion);
      Screen.print(3, buff);
      break;
     case 4:
     Screen.print(1, "Per million:");
      makeLine(buff," Active %d",stats[country].activePerOneMillion);
      Screen.print(2, buff);
      makeLine(buff," Critical %d",stats[country].criticalPerOneMillion);
      Screen.print(3, buff);
      break;
    case 5:
      makeLine(buff,"Tests %d",stats[country].tests);
      Screen.print(1, buff);
      makeLine(buff,"Per mil %d",stats[country].testsPerOneMillion);
      Screen.print(2, buff);
      Screen.print(3, " ");
      break;
    }
}

void setup() {
  hasWifi = false;

  //Serial.begin(115200);
  
  Screen.clean();
  Screen.print(0, "COVID-19");
  Screen.print(1, "Connecting");
  Screen.print(2, "to");
  Screen.print(3, "Wi-Fi");
  
  InitWiFi();

  init=1; // trigger the reading of data

}

void loop() {  
  if (!hasWifi) {
    Screen.print(1, "No Wi-Fi available",true);
  } else if (init==1) {
    Screen.print(0, "COVID-19");
    Screen.print(1, "Gathering");
    Screen.print(2, "Data");
    Screen.print(3, " ");
    lastCountry=-1;
    //http_get("all");
    http_get("countries/ie");
    init++;
  } else if (init==2) {
    http_get("countries/usa");
    init++;    
  } else if (init==3) {
    http_get("countries/uk");
    init++;
  } else if (init==4) {
    http_get("all");
    init=0;
    Screen.print(1, "Press A to change country, B to change metrics",true);
  } else if (getButtonAState() && getButtonBState()) {
    init=1; //force the reloading of data
    pressedA=pressedB=true;
  } else if (getButtonAState() && pressedA) {
    //wait for button A release
  } else if (getButtonBState() && pressedB) {
    //wait for button B release
  } else if (getButtonAState()) {
    //respond to button A being pressed
    country+=1;
    if (country>lastCountry) country=0;
    //screen=0; uncomment if you want to go to the first screen for the new country
    showStats();
    pressedA=true;
  } else if (getButtonBState()) {
    //respond to button B being pressed
    if (country<0) country=0;
    screen+=1;
    if (screen>5) screen=0;
    showStats();
    pressedB=true;
  }

  if (!getButtonAState()) pressedA=false;
  if (!getButtonBState()) pressedB=false;

}
