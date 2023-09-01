#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include <time.h>
#include <HTTPClient.h>
// port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// 磁簧開關針腳設定
int switchReed = 39;
int switchReed2 = 35;

// wifi設定
const char *ssid = "AGIT";
const char *pwd = "BarkingCrab";
String doorStatus = "Loading...";
float flag;

// 時間設定
const char *ntpserver = "pool.ntp.org";
const uint16_t utcOffest = 28800;
const uint8_t daylightOffest = 0;
String year, month, day, currentSec, currentMin, currentHour; // 年, 月, 日, 秒, 分, 時
String dateValue;                                             // 取得時間
bool isHoliday;                                               // 判定是否假日
HTTPClient http;

// 睡眠時間
bool Timesleep = false;           // 睡眠的觸發
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 46800           /* Time ESP32 will go to sleep (in seconds)     早上7:58分到晚上20:00*/

// 拿取網路上放假的日期
void get_holiday_json(String year, String month, String day)
{
  while (true)
  {
    String url = "https://cdn.jsdelivr.net/gh/ruyut/TaiwanCalendar/data/" + year + ".json";
    DynamicJsonDocument doc(49152); // json解析大小
    http.begin(url);                // 指定網址
    int httpCode = http.GET();      // 發請連結請求

    if (httpCode > 0)
    {
      String payload = http.getString(); // 回傳http 本體(字串格式)

      // 解析 JSON
      DeserializationError error = deserializeJson(doc, payload);
      if (!error)
      {

        String targetDate = year + month + day;

        for (int i = 0; i < doc.size(); i++)
        {
          String dateValue = doc[i]["date"].as<String>();
          if (dateValue == targetDate)
          {
            Serial.println("今天日期：" + dateValue);
            bool isHoliday = doc[i]["isHoliday"].as<bool>();
            Serial.println("是否為放假：" + String(isHoliday));
            return;
          }
        }
      }
      else
      {
        Serial.println("JSON 解析錯誤");
      }
    }
    else
    {
      Serial.println("HTTP 錯誤");
    }
    delay(5000);
  }
}

// 偵測這裡的wifi訊號強弱
void show_how_strong_wifi()
{
  int wifi_sensor;
  wifi_sensor = WiFi.RSSI();
  Serial.print("WIFI強度:");
  Serial.println(WiFi.RSSI());

  if (wifi_sensor < -80)
  {
    Serial.println("這邊訊號很爛，你趕緊換個地方吧");
  }
  else if (wifi_sensor > -80 && wifi_sensor < -67)
  {
    Serial.println("這邊還行，勉強，但偏危險");
  }
  else
  {
    Serial.println("哦~很強很強，放在這裡吧");
  }
}

String html()
{
  String HTML = "<!DOCTYPE html>\
<html>\
<head>\
  <title>Device Monitor</title>\
  <meta charset='UTF-8'>\
</head>\
<body>\
  <strong>今天日期:" +
                dateValue + "</strong>\
  <h1>Device Status: <span id='status'>" +
                doorStatus + "</span></h1>\
  <h1>裝置狀態(開 : 1 關 : 0): <span id='door_status_number'>[" +
                flag + "]</span></h1>\
  <script>\
    var socket = new WebSocket('ws://' + window.location.host + '/ws');\
    socket.onmessage = function(event) {\
      var data = JSON.parse(event.data); \
      document.getElementById('status').innerText = data.doorStatus;\
      document.getElementById('door_status_number').innerText = '[' + data.flag + ']';\
    };\
  </script>\
</body>\
</html>";

  return HTML;
}
/*
AsyncWebSocket *server：指向AsyncWebSocket物件的指標。這個參數代表WebSocket伺服器的實例，您可以使用它來設定或管理WebSocket伺服器的相關設定。

AsyncWebSocketClient *client：指向AsyncWebSocketClient物件的指標。這個參數代表與WebSocket伺服器連線的客戶端。您可以使用它來與特定客戶端進行通訊。

AwsEventType type：這是一個列舉型別的參數，表示事件的類型。可以根據這個參數來判斷何種類型的事件發生，例如連線、斷線、接收到文字訊息等。

void *arg：這是一個指向額外參數的指標。在某些事件類型中，可能會傳遞額外的參數給回呼函式，這個指標可以用來存取這些額外的參數。

uint8_t *data：這是一個指向數據緩衝區的指標。在某些事件類型中，可能會傳遞數據給回呼函式，這個指標可以用來存取這些數據。

size_t len：這是數據的長度，表示data緩衝區中的數據大小。
*/
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  // 接收WebSocket事件
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("連線成功");
    Serial.print("Ip Address: ");
    Serial.println(WiFi.localIP());
  }

  pinMode(switchReed, INPUT);
  pinMode(switchReed2, INPUT);

  // 顯示網路訊號強弱
  show_how_strong_wifi();

  // 設置取得網路時間
  configTime(utcOffest, daylightOffest, ntpserver);

  struct tm now;

  if (!getLocalTime(&now))
  {
    Serial.println("無法取得時間~");
  }
  else
  {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &now);

    year = String(buffer + 0, 4);         // 年份
    month = String(buffer + 5, 2);        // 月份
    day = String(buffer + 8, 2);          // 日期
    currentHour = String(buffer + 10, 2); // 時10
    currentMin = String(buffer + 0, 4);   // 分
    currentSec = String(buffer + 0, 4);   // 秒
  }

  get_holiday_json(year, month, day);
  // Server請求路徑, 以及相對應的響應方式
  /* server.on("/test", [](AsyncWebServerRequest *request)
              { request->send(200, "text/html; charset=utf-8", "跨沙小"); });
    server.on("/monitor", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html; charset=utf-8", doorStatus); });
  */
  // 將HTML內容發送到瀏覽器
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String status = doorStatus;
    float door_status_number = flag;

    request -> send(200, "text/html; charset=utf-8", html()); });

  /*
  ws.onEvent(onWebSocketEvent)：這行程式碼設定了WebSocket的事件處理器。當WebSocket伺服器收到任何事件時，都會呼叫指定的事件處理器函式。在這裡，
                                我們指定了onWebSocketEvent函式作為事件處理器。

  server.addHandler(&ws)：這行程式碼將WebSocket伺服器加入到AsyncWebServer的事件處理器中。這樣做是為了讓WebSocket伺服器能夠接收和處理來自客戶端
                          的WebSocket連線。
  */
  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);
  server.begin();
}
int newHour = 0;
// 偵測開關
void loop()
{

  if (Timesleep)
  {
    Serial.println("進入睡眠模式...");
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
    return; // 不需要在睡眠 跳出迴圈
  }

  struct tm now;
  if (getLocalTime(&now))
  {
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y/%m/%d", &now);
    dateValue = String(buffer);
    Serial.println(dateValue);
    if (now.tm_hour == 0 && now.tm_min == 0 && now.tm_sec == 0)
    {
      year = String(buffer + 0, 4);  // 年份
      month = String(buffer + 5, 2); // 月份
      day = String(buffer + 8, 2);   // 日期
      get_holiday_json(year, month, day);
    }
  }

  int currentHour = now.tm_hour;
  int currentMinute = now.tm_min;
  int currentSec = now.tm_sec;
  Serial.print(currentHour);
  Serial.print(":");
  Serial.print(currentMinute);
  Serial.print(":");
  Serial.print(currentSec);
  Serial.print("\n");

  if (!isHoliday)
  {
    if (currentHour == 7)
    {
      Timesleep = true;
      return;
    }
    else
    {
      if (currentHour >= 7 || currentHour < 13)
      {
        if (digitalRead(switchReed) == 0 || digitalRead(switchReed2) == 0)
        {
          Serial.println("Window Open Port2");
          doorStatus = "C棟一樓門被打開 ";
          flag = 1.0;
        }
        else if (digitalRead(switchReed) == 1 && digitalRead(switchReed2) == 1)
        {
          Serial.println("Window Closed");
          doorStatus = "C棟一樓門被關閉";
          flag = 0.0;
        }
      }
    }
  }
  else
  {

    if (digitalRead(switchReed) == 0 || digitalRead(switchReed2) == 0)
    {
      Serial.println("Window Open Port2");
      doorStatus = "C棟一樓門被打開 ";
      flag = 1.0;
    }
    else if (digitalRead(switchReed) == 1 && digitalRead(switchReed2) == 1)
    {
      Serial.println("Window Closed");
      doorStatus = "C棟一樓門被關閉";
      flag = 0.0;
    }
  }

  // ws.textAll(doorStatus);
  // ws.textAll(String(flag));
  StaticJsonDocument<128> jsonDoc;
  jsonDoc["doorStatus"] = doorStatus;
  jsonDoc["flag"] = flag;
  String jsonData;
  serializeJson(jsonDoc, jsonData);

  ws.textAll(jsonData); // Send JSON data to all clients

  delay(1000);
}