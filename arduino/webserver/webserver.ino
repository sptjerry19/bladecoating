#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Thông tin WiFi (Station Mode - Kết nối WiFi hiện có)
const char* ssid_sta = "cheese";
const char* password_sta = "88888888";

// Thông tin WiFi (Access Point Mode - Tự phát WiFi)
const char* ssid_ap = "blade_coating";
const char* password_ap = "12345678";

// Khởi tạo server
ESP8266WebServer server(80);

// Các lệnh điều khiển
String command = "";

// Biến lưu trữ tốc độ motor nhận được
String motorSpeed = "0.0";

// Hàm xử lý trang chủ
void handleRoot() {
  String html = "<html><head><meta http-equiv='refresh' content='2'/><title>Motor Speed</title></head><body>";
  html += "<h1>Motor Speed Monitor</h1>";
  html += "<p>Current Speed: " + motorSpeed + " steps/s</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// Thiết lập kết nối WiFi (STA + AP)
void setup() {
  Serial.begin(9600);

  // Chế độ Station Mode
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_sta, password_sta);

  // Chờ kết nối WiFi
  Serial.println("Đang kết nối WiFi (STA)...");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nĐã kết nối WiFi (STA)!");
    Serial.print("Địa chỉ IP (STA): ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nKhông kết nối được WiFi (STA).");
  }

  // Chế độ Access Point
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("Access Point đã sẵn sàng.");
  Serial.print("Địa chỉ IP (AP): ");
  Serial.println(WiFi.softAPIP());

  // Định nghĩa các route
  server.on("/", handleRoot);
  // server.on("/control", handleControl);

  // Bắt đầu server
  server.begin();
  Serial.println("Server bắt đầu hoạt động...");
}

// Hiển thị giao diện IoT


void loop() {
  server.handleClient();
  
  // Kiểm tra dữ liệu nhận từ Arduino qua Serial
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    // Giả sử dữ liệu có định dạng: "Speed:xxx"
    if (data.startsWith("Speed:")) {
      motorSpeed = data.substring(6); // Cắt phần sau "Speed:"
      motorSpeed.trim();
      Serial.print("Received speed: ");
      Serial.println(motorSpeed);
    }
  }
}
