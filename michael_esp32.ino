  #include <Arduino.h>
  #include <Wire.h>
  #include <WiFi.h>
  #include <WebServer.h>
  #include <Preferences.h>
  #include <LiquidCrystal_I2C.h>
  #include <WiFiManager.h>
  #include <NTPClient.h>
  #include <WiFiUdp.h>

  // Define relay pins (active LOW)
  #define RELAY1_PIN 2
  #define RELAY2_PIN 15
  #define RELAY3_PIN 23
  #define RELAY4_PIN 17

  // Define switch pins
  #define SWITCH1_PIN 13
  #define SWITCH2_PIN 12 
  #define SWITCH3_PIN 18
  #define SWITCH4_PIN 19

  // Initialize I2C LCD (20x4)
  LiquidCrystal_I2C lcd(0x27, 20, 4);

  // Initialize NTP client (UTC+8 for Philippines)
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "ntp.pagasa.dost.gov.ph", 28800, 60000);

  Preferences preferences;
  WiFiManager wifiManager;
  WebServer server(80);

  // Time settings for each relay
  int onHour1, onMinute1, offHour1, offMinute1;
  int onHour2, onMinute2, offHour2, offMinute2;
  int onHour3, onMinute3, offHour3, offMinute3;
  int onHour4, onMinute4, offHour4, offMinute4;

  // Relay states
  bool relay1State = false;
  bool relay2State = false;
  bool relay3State = false;
  bool relay4State = false;

  const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Set Relay Times</title>
    <style>
      body { text-align: center; font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }
      h1, h2 { color: #444; }
      .relay-container { display: flex; flex-wrap: wrap; justify-content: space-between; margin-bottom: 20px; }
      .relay-pair { display: flex; width: 100%; margin-bottom: 30px; }
      .relay-box { flex: 1; padding: 15px; margin: 0 10px; border: 1px solid #ccc; border-radius: 5px; min-width: 250px; }
      input[type="text"] { padding: 8px; margin: 5px 0; width: 80%; }
      input[type="submit"] { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 20px; }
      input[type="submit"]:hover { background-color: #45a049; }
      .status { margin-top: 30px; padding: 15px; background-color: #f0f0f0; border-radius: 5px; text-align: center; }
      a { color: #0066cc; text-decoration: none; }
      a:hover { text-decoration: underline; }
    </style>
  </head>
  <body>
    <h1>Activity 4 RTOS ESP32</h1>
    <h2>Set Relay Times</h2>
    <form action="/set" method="POST">
      <div class="relay-container">
        <div class="relay-pair">
          <div class="relay-box">
            <h3>Relay 1</h3>
            ON Time (HH:MM):<br><input type="text" name="onTime1" placeholder="08:00"><br>
            OFF Time (HH:MM):<br><input type="text" name="offTime1" placeholder="18:00"><br>
          </div>
          <div class="relay-box">
            <h3>Relay 2</h3>
            ON Time (HH:MM):<br><input type="text" name="onTime2" placeholder="09:00"><br>
            OFF Time (HH:MM):<br><input type="text" name="offTime2" placeholder="19:00"><br>
          </div>
        </div>
        <div class="relay-pair">
          <div class="relay-box">
            <h3>Relay 3</h3>
            ON Time (HH:MM):<br><input type="text" name="onTime3" placeholder="10:00"><br>
            OFF Time (HH:MM):<br><input type="text" name="offTime3" placeholder="20:00"><br>
          </div>
          <div class="relay-box">
            <h3>Relay 4</h3>
            ON Time (HH:MM):<br><input type="text" name="onTime4" placeholder="11:00"><br>
            OFF Time (HH:MM):<br><input type="text" name="offTime4" placeholder="21:00"><br>
          </div>
        </div>
      </div>
      <input type="submit" value="Set All Times">
    </form>
    <div class="status">
      <p>Current IP: %IP%</p>
      <p>SSID: %SSID%</p>
      <p><a href="/reset">Reset WiFi Settings</a></p>
    </div>
  </body>
  </html>
  )rawliteral";

  String processor(const String& var) {
    if (var == "IP") return WiFi.localIP().toString();
    if (var == "SSID") return WiFi.SSID();
    return String();
  }

  void handleRoot() {
    String html = index_html;
    html.replace("%IP%", WiFi.localIP().toString());
    html.replace("%SSID%", WiFi.SSID());
    server.send(200, "text/html", html);
  }

  void handleSet() {
    String onTime1 = server.arg("onTime1");
    String offTime1 = server.arg("offTime1");
    String onTime2 = server.arg("onTime2");
    String offTime2 = server.arg("offTime2");
    String onTime3 = server.arg("onTime3");
    String offTime3 = server.arg("offTime3");
    String onTime4 = server.arg("onTime4");
    String offTime4 = server.arg("offTime4");

    onHour1 = onTime1.substring(0, 2).toInt();
    onMinute1 = onTime1.substring(3).toInt();
    offHour1 = offTime1.substring(0, 2).toInt();
    offMinute1 = offTime1.substring(3).toInt();
    onHour2 = onTime2.substring(0, 2).toInt();
    onMinute2 = onTime2.substring(3).toInt();
    offHour2 = offTime2.substring(0, 2).toInt();
    offMinute2 = offTime2.substring(3).toInt();
    onHour3 = onTime3.substring(0, 2).toInt();
    onMinute3 = onTime3.substring(3).toInt();
    offHour3 = offTime3.substring(0, 2).toInt();
    offMinute3 = offTime3.substring(3).toInt();
    onHour4 = onTime4.substring(0, 2).toInt();
    onMinute4 = onTime4.substring(3).toInt();
    offHour4 = offTime4.substring(0, 2).toInt();
    offMinute4 = offTime4.substring(3).toInt();

    preferences.begin("relay_settings", false);
    preferences.putUInt("R1_onHour", onHour1);
    preferences.putUInt("R1_onMinute", onMinute1);
    preferences.putUInt("R1_offHour", offHour1);
    preferences.putUInt("R1_offMinute", offMinute1);
    preferences.putUInt("R2_onHour", onHour2);
    preferences.putUInt("R2_onMinute", onMinute2);
    preferences.putUInt("R2_offHour", offHour2);
    preferences.putUInt("R2_offMinute", offMinute2);
    preferences.putUInt("R3_onHour", onHour3);
    preferences.putUInt("R3_onMinute", onMinute3);
    preferences.putUInt("R3_offHour", offHour3);
    preferences.putUInt("R3_offMinute", offMinute3);
    preferences.putUInt("R4_onHour", onHour4);
    preferences.putUInt("R4_onMinute", onMinute4);
    preferences.putUInt("R4_offHour", offHour4);
    preferences.putUInt("R4_offMinute", offMinute4);
    preferences.end();

    String message = "<html><body><h1>Times set successfully!</h1>";
    message += "<p><a href='/'>Return to control panel</a></p>";
    message += "</body></html>";
    server.send(200, "text/html", message);
  }

  void handleReset() {
    wifiManager.resetSettings();
    String message = "<html><body><h1>WiFi settings reset!</h1>";
    message += "<p>The device will restart in configuration mode.</p>";
    message += "<p>Connect to the ESP32 access point to configure new WiFi settings.</p>";
    message += "</body></html>";
    server.send(200, "text/html", message);
    delay(3000);
    ESP.restart();
  }

  void handleToggle() {
    String relayNum = server.arg("relay");
    String state = server.arg("state");
    
    int relayIndex = relayNum.toInt() - 1;
    bool newState = (state == "1");
    
    switch(relayIndex) {
      case 0:
        relay1State = newState;
        digitalWrite(RELAY1_PIN, newState ? LOW : HIGH);
        break;
      case 1:
        relay2State = newState;
        digitalWrite(RELAY2_PIN, newState ? LOW : HIGH);
        break;
      case 2:
        relay3State = newState;
        digitalWrite(RELAY3_PIN, newState ? LOW : HIGH);
        break;
      case 3:
        relay4State = newState;
        digitalWrite(RELAY4_PIN, newState ? LOW : HIGH);
        break;
      default:
        server.send(400, "text/plain", "Invalid relay number");
        return;
    }
    
    String message = "<html><body><h1>Relay toggled!</h1>";
    message += "<p><a href='/'>Return to control panel</a></p>";
    message += "</body></html>";
    server.send(200, "text/html", message);
  }

  void setup() {
    Serial.begin(9600);
    Wire.begin();

    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");

    pinMode(RELAY1_PIN, OUTPUT);
    digitalWrite(RELAY1_PIN, HIGH);
    pinMode(RELAY2_PIN, OUTPUT);
    digitalWrite(RELAY2_PIN, HIGH);
    pinMode(RELAY3_PIN, OUTPUT);
    digitalWrite(RELAY3_PIN, HIGH);
    pinMode(RELAY4_PIN, OUTPUT);
    digitalWrite(RELAY4_PIN, HIGH);

    pinMode(SWITCH1_PIN, INPUT_PULLUP);
    pinMode(SWITCH2_PIN, INPUT_PULLUP);
    pinMode(SWITCH3_PIN, INPUT_PULLUP);
    pinMode(SWITCH4_PIN, INPUT_PULLUP);

    lcd.setCursor(0, 1);
    lcd.print("Connecting WiFi...");
    wifiManager.setConfigPortalTimeout(180);
    wifiManager.setConnectTimeout(30);
    if (!wifiManager.autoConnect("ESP32_Relay_Controller")) {
      Serial.println("Failed to connect and hit timeout");
      lcd.clear();
      lcd.print("WiFi Connect Failed");
      lcd.setCursor(0, 1);
      lcd.print("Restarting...");
      delay(3000);
      ESP.restart();
    }

    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print("IP: ");
    lcd.print(WiFi.localIP());
    delay(2000);

    timeClient.begin();

    preferences.begin("relay_settings", true);
    onHour1 = preferences.getUInt("R1_onHour", 8);
    onMinute1 = preferences.getUInt("R1_onMinute", 0);
    offHour1 = preferences.getUInt("R1_offHour", 18);
    offMinute1 = preferences.getUInt("R1_offMinute", 0);
    onHour2 = preferences.getUInt("R2_onHour", 9);
    onMinute2 = preferences.getUInt("R2_onMinute", 0);
    offHour2 = preferences.getUInt("R2_offHour", 19);
    offMinute2 = preferences.getUInt("R2_offMinute", 0);
    onHour3 = preferences.getUInt("R3_onHour", 10);
    onMinute3 = preferences.getUInt("R3_onMinute", 0);
    offHour3 = preferences.getUInt("R3_offHour", 20);
    offMinute3 = preferences.getUInt("R3_offMinute", 0);
    onHour4 = preferences.getUInt("R4_onHour", 11);
    onMinute4 = preferences.getUInt("R4_onMinute", 0);
    offHour4 = preferences.getUInt("R4_offHour", 21);
    offMinute4 = preferences.getUInt("R4_offMinute", 0);
    preferences.end();

    server.on("/", handleRoot);
    server.on("/set", handleSet);
    server.on("/reset", handleReset);
    server.on("/toggle", handleToggle);
    server.begin();
    Serial.println("HTTP server started");
  }

  void loop() {
    if (digitalRead(SWITCH1_PIN) == LOW) {
      relay1State = !relay1State;
      digitalWrite(RELAY1_PIN, relay1State ? LOW : HIGH);
      delay(200);
    }
    if (digitalRead(SWITCH2_PIN) == LOW) {
      relay2State = !relay2State;
      digitalWrite(RELAY2_PIN, relay2State ? LOW : HIGH);
      delay(200);
    }
    if (digitalRead(SWITCH3_PIN) == LOW) {
      relay3State = !relay3State;
      digitalWrite(RELAY3_PIN, relay3State ? LOW : HIGH);
      delay(200);
    }
    if (digitalRead(SWITCH4_PIN) == LOW) {
      relay4State = !relay4State;
      digitalWrite(RELAY4_PIN, relay4State ? LOW : HIGH);
      delay(200);
    }

    server.handleClient();
    timeClient.update();

    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    updateRelayState(currentHour, currentMinute);
    displayData(currentHour, currentMinute, currentSecond);

    delay(1000);
  }

  void updateRelayState(int hour, int minute) {
    if (hour == onHour1 && minute == onMinute1) {
      relay1State = true;
      digitalWrite(RELAY1_PIN, LOW);
    } else if (hour == offHour1 && minute == offMinute1) {
      relay1State = false;
      digitalWrite(RELAY1_PIN, HIGH);
    }
    if (hour == onHour2 && minute == onMinute2) {
      relay2State = true;
      digitalWrite(RELAY2_PIN, LOW);
    } else if (hour == offHour2 && minute == offMinute2) {
      relay2State = false;
      digitalWrite(RELAY2_PIN, HIGH);
    }
    if (hour == onHour3 && minute == onMinute3) {
      relay3State = true;
      digitalWrite(RELAY3_PIN, LOW);
    } else if (hour == offHour3 && minute == offMinute3) {
      relay3State = false;
      digitalWrite(RELAY3_PIN, HIGH);
    }
    if (hour == onHour4 && minute == onMinute4) {
      relay4State = true;
      digitalWrite(RELAY4_PIN, LOW);
    } else if (hour == offHour4 && minute == offMinute4) {
      relay4State = false;
      digitalWrite(RELAY4_PIN, HIGH);
    }
  }

  String formatTime(int hour, int minute) {
    String h = (hour < 10) ? "0" + String(hour) : String(hour);
    String m = (minute < 10) ? "0" + String(minute) : String(minute);
    return h + ":" + m;
  }

  void displayData(int hour, int minute, int second) {
    String timeStr = "Time: " + formatTime(hour, minute) + ":" + 
                    (second < 10 ? "0" + String(second) : String(second)) + 
                    " W:" + (WiFi.status() == WL_CONNECTED ? "ON" : "OFF");
    String R1Str = "R1ON:" + formatTime(onHour1, onMinute1) + " OFF:" + formatTime(offHour1, offMinute1);
    String R2Str = "R2ON:" + formatTime(onHour2, onMinute2) + " OFF:" + formatTime(offHour2, offMinute2);
    String R3Str = "R3ON:" + formatTime(onHour3, onMinute3) + " OFF:" + formatTime(offHour3, offMinute3);
    String R4Str = "R4ON:" + formatTime(onHour4, onMinute4) + " OFF:" + formatTime(offHour4, offMinute4);
    String ipStr = "IP: " + WiFi.localIP().toString();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(timeStr.substring(0, 20));

    if (second % 10 < 5) {
      lcd.setCursor(0, 1);
      lcd.print(R1Str.substring(0, 20));
      lcd.setCursor(0, 2);
      lcd.print(R2Str.substring(0, 20)); // Fixed typo here
    } else {
      lcd.setCursor(0, 1);
      lcd.print(R3Str.substring(0, 20));
      lcd.setCursor(0, 2);
      lcd.print(R4Str.substring(0, 20));
    }

    lcd.setCursor(0, 3);
    lcd.print(ipStr.substring(0, 20));
  }