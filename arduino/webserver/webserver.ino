#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ====================
// PHẦN 1: Điều khiển động cơ bước và thu thập dữ liệu
// ====================

// Cấu hình LCD I2C (địa chỉ 0x27, kích thước 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Định nghĩa chân cho driver động cơ bước (kiểu DRIVER: cần STEP và DIR)
#define STEP_PIN 8
#define DIR_PIN 9

// Khởi tạo đối tượng AccelStepper (chế độ DRIVER)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// --- Thông số cơ học ---
// Số bước/vòng (theo yêu cầu mới)
const int mech_stepsPerRevolution = 6400;
// Vít me: 5 mm/1 vòng
const float leadScrewPitch = 5.0;
// Quãng đường di chuyển (mm) – bạn có thể thay đổi thông số này
float distanceToMove = 200.0;
// Thời gian di chuyển (giây) – sẽ được nhập từ Serial (hoặc chỉnh sửa)
float timeToMove = 10.0;
// Tính tổng số bước cần thiết
long totalSteps = (distanceToMove / leadScrewPitch) * mech_stepsPerRevolution;
// Tốc độ mục tiêu (bước/giây) được tính lại sau khi có thời gian
float targetSpeed = totalSteps / timeToMove;
// Gia tốc (step/s²)
int motorAcceleration = targetSpeed / 2;  // có thể thay đổi

// --- Chuỗi JSON lưu dữ liệu mẫu ---
String jsonData = "";
bool firstSample = true;
int sampleCounter = 0;

// Hàm lấy mẫu (mỗi giây) – ghi thời gian (số mẫu), tốc độ hiện tại (mm/s) và quãng đường (mm)
void recordSample() {
  sampleCounter++;
  // Lấy tốc độ hiện tại (bước/s) và đổi sang mm/s:
  float currentSpeedSteps = stepper.speed();
  float currentSpeedMM = currentSpeedSteps * leadScrewPitch / mech_stepsPerRevolution;
  // Tính quãng đường di chuyển (mm)
  float currentDistanceMM = abs(stepper.currentPosition()) * leadScrewPitch / mech_stepsPerRevolution;
  
  if (!firstSample) {
    jsonData += ",\n";
  }
  jsonData += "  { \"time\": " + String(sampleCounter) +
              ", \"speed\": " + String(currentSpeedMM, 1) +
              ", \"distance\": " + String(currentDistanceMM, 1) + " }";
  firstSample = false;
}

// Hàm chạy động cơ với lấy mẫu dữ liệu trong suốt quá trình chuyển động  
// target: vị trí mục tiêu (số bước)  
// moveDuration: thời gian chuyển động dự kiến (ms)
void moveWithSampling(long target, unsigned long moveDuration) {
  stepper.moveTo(target);
  unsigned long startTime = millis();
  unsigned long lastSampleTime = millis();
  
  while (stepper.distanceToGo() != 0) {
    stepper.run(); // Điều khiển động cơ với gia tốc giảm tốc tự động
    if (millis() - lastSampleTime >= 1000) {  // lấy mẫu mỗi 1 giây
      recordSample();
      lastSampleTime = millis();
    }
  }
  // Nếu thời gian chạy chưa đủ moveDuration, đợi thêm
  unsigned long elapsed = millis() - startTime;
  if (elapsed < moveDuration) {
    delay(moveDuration - elapsed);
  }
}

// Hàm thực hiện chuyển động: tiến (đi theo quãng đường đã chọn) rồi quay lại vị trí ban đầu  
// Hàm này sẽ xây dựng chuỗi JSON và in ra Serial (để ESP8266 phục vụ web)
void runMotorMotion() {
  // Cập nhật lại các thông số tính toán dựa trên giá trị hiện hành
  totalSteps = (distanceToMove / leadScrewPitch) * mech_stepsPerRevolution;
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(motorAcceleration);
  
  // Reset chuỗi JSON và bộ đếm mẫu
  jsonData = "[\n";
  firstSample = true;
  sampleCounter = 0;
  
  // Hiển thị trên LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moving Forward");
  
  // Chạy chuyển động tiến
  moveWithSampling(totalSteps, (unsigned long)(timeToMove * 1000));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Returning Back");
  
  // Chạy chuyển động quay lại vị trí ban đầu
  moveWithSampling(0, (unsigned long)(timeToMove * 1000));
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Operation Done!");
  delay(1000);
  
  // Kết thúc chuỗi JSON
  jsonData += "\n]";
  
  // Gửi JSON qua Serial để web có thể lấy dữ liệu
  Serial.println(jsonData);
}

// ====================
// PHẦN 2: Server Web (ESP8266) phục vụ giao diện và dữ liệu
// ====================

// Thông tin WiFi – thay đổi theo mạng của bạn
const char* ssid_sta = "cheese";
const char* password_sta = "88888888";

const char* ssid_ap = "blade_coating";
const char* password_ap = "12345678";

// Khởi tạo web server trên cổng 80
ESP8266WebServer server(80);

// Biến lưu trữ dữ liệu JSON nhận được từ phần chuyển động (Arduino)
String motorData = "[]";  // mặc định mảng rỗng

// Giao diện web – sử dụng raw literal để dễ quản lý
const char htmlControl[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Điều khiển động cơ bước</title>
    <style>
      /* CSS của bạn */
    </style>
  </head>
  <body>
    <!-- Nội dung HTML -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
      // Các hàm JavaScript của bạn, ví dụ:
      function updateChart(dataArray) {
        monitoringChart.data.labels = [];
        monitoringChart.data.datasets[0].data = [];
        dataArray.forEach(item => {
          monitoringChart.data.labels.push(item.time);
          monitoringChart.data.datasets[0].data.push(item.speed);
        });
        monitoringChart.update();
      }
      function fetchData() {
        fetch('/data')
          .then(response => response.json())
          .then(data => {
            console.log("Dữ liệu nhận từ Arduino:", data);
            updateChart(data);
          })
          .catch(err => console.error("Lỗi khi lấy dữ liệu:", err));
      }
      setInterval(fetchData, 3000);
      function startOperation() {
        fetch('/start')
          .then(response => response.text())
          .then(data => { console.log("Đã bắt đầu: " + data); })
          .catch(err => console.error("Lỗi khi bắt đầu:", err));
      }
    </script>
  </body>
</html>
)rawliteral";

  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Điều khiển động cơ bước</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 0;
        background-color: #f4f4f9;
        color: #333;
        text-align: center;
      }
      h1 {
        margin: 20px 0;
        font-size: 24px;
        color: #2c3e50;
      }
      .container {
        padding: 20px;
        max-width: 500px;
        margin: 30px auto;
        background: #ffffff;
        box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
        border-radius: 10px;
      }
      .button-group {
        margin: 15px 0;
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
      }
      button {
        padding: 10px 15px;
        margin: 5px;
        font-size: 16px;
        color: #fff;
        background-color: #3498db;
        border: none;
        border-radius: 5px;
        cursor: pointer;
        transition: background-color 0.3s ease;
      }
      button:hover {
        background-color: #2980b9;
      }
      .input-group {
        margin: 15px 0;
      }
      .input-group label {
        display: block;
        margin-bottom: 5px;
        font-size: 16px;
        font-weight: bold;
        text-align: left;
      }
      .input-group input {
        width: calc(100% - 20px);
        padding: 10px;
        font-size: 16px;
        margin: 0 auto;
        border: 1px solid #ccc;
        border-radius: 5px;
        display: block;
      }
      .info {
        margin-top: 20px;
        font-size: 18px;
        text-align: left;
      }
      .info div {
        margin: 10px 0;
        padding: 10px;
        background: #ecf0f1;
        border-radius: 5px;
      }
      .chart-container {
        margin: 20px auto;
        max-width: 600px;
      }
      .data-container {
        margin-top: 20px;
      }
      @media (max-width: 768px) {
        .container {
          padding: 15px;
        }
        button {
          font-size: 14px;
          padding: 10px;
        }
        .info div {
          font-size: 16px;
        }
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h1>Điều khiển động cơ bước</h1>

      <!-- Nhập quãng đường và tốc độ -->
      <div class="input-group">
        <label for="distanceInput">Nhập quãng đường (mm):</label>
        <input
          type="number"
          id="distanceInput"
          min="0"
          placeholder="Nhập quãng đường..."
        />
      </div>

      <div class="input-group">
        <label for="speedInput">Nhập tốc độ (mm/s):</label>
        <input
          type="number"
          id="speedInput"
          min="0"
          placeholder="Nhập tốc độ..."
        />
      </div>

      <button onclick="setParameters()">Cập nhật thông số</button>

      <div class="button-group">
        <button onclick="startTimer()">Bắt đầu</button>
        <button onclick="stopTimer()">Dừng</button>
        <button onclick="resetSystem()">Reset</button>
      </div>

      <!-- Hiển thị thông tin -->
      <div class="info">
        <h2>Thông số động cơ</h2>
        <div><strong>Tốc độ:</strong> <span id="speed">0</span> mm/s</div>
        <div><strong>Quãng đường:</strong> <span id="distance">0</span> mm</div>
        <div>
          <strong>Gia tốc:</strong> <span id="acceleration">0</span> mm/s²
        </div>
        <div>
          <strong>Thời gian hoàn thành:</strong> <span id="time">0</span> s
        </div>
      </div>

      <!-- Biểu đồ giám sát -->
      <div class="chart-container">
        <h2>Biểu đồ giám sát</h2>
        <canvas id="monitoringChart" width="400" height="200"></canvas>
      </div>

      <!-- Lưu trữ dữ liệu -->
      <div class="data-container">
        <h2>Lưu trữ dữ liệu</h2>
        <button onclick="exportData()">Xuất dữ liệu</button>
        <div id="dataLogs"></div>
      </div>
    </div>

    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
      // Thông số vít-me
      const leadScrewPitch = 5; // mm/rev
      const stepsPerRevolution = 200; // step/rev (ví dụ; nếu cần có thể thay đổi)
      const microstep = 16; // vi bước (nếu sử dụng)

      let speed = 0; // mm/s
      let acceleration = 0; // mm/s²
      let distance = 0; // mm
      let time = 0; // giây
      let countdown; // biến đếm ngược
      let dataLogs = []; // lưu trữ dữ liệu
      let monitoringChart; // biến biểu đồ

      // Khởi tạo biểu đồ với Chart.js
      const ctx = document.getElementById("monitoringChart").getContext("2d");
      monitoringChart = new Chart(ctx, {
        type: "line",
        data: {
          labels: [],
          datasets: [
            {
              label: "Tốc độ (mm/s)",
              data: [],
              borderColor: "#3498db",
              fill: false,
            },
          ],
        },
        options: {
          responsive: true,
          scales: {
            x: {
              title: {
                display: true,
                text: "Thời gian (s)",
              },
            },
            y: {
              title: {
                display: true,
                text: "Tốc độ (mm/s)",
              },
            },
          },
        },
      });

      // Tính toán thông số động cơ
      function calculateParameters() {
        const inputDistance = parseFloat(
          document.getElementById("distanceInput").value
        );
        const inputSpeed = parseFloat(
          document.getElementById("speedInput").value
        );
        if (
          isNaN(inputDistance) ||
          inputDistance <= 0 ||
          isNaN(inputSpeed) ||
          inputSpeed <= 0
        ) {
          alert("Vui lòng nhập quãng đường và tốc độ hợp lệ.");
          return;
        }
        distance = inputDistance;
        speed = inputSpeed;
        // Tính gia tốc và thời gian hoàn thành
        acceleration = (speed * speed) / (2 * distance);
        time = distance / speed;
        updateDisplay();
      }

      // Cập nhật hiển thị thông số lên trang
      function updateDisplay() {
        document.getElementById("speed").innerText = speed.toFixed(2);
        document.getElementById("distance").innerText = distance.toFixed(2);
        document.getElementById("acceleration").innerText =
          acceleration.toFixed(2);
        document.getElementById("time").innerText = time.toFixed(2);
      }

      // Gọi tính toán và gửi thông số đến ESP8266
      function setParameters() {
        calculateParameters();
        fetchParameters();
      }

      // Bắt đầu chuyển động và cập nhật biểu đồ theo thời gian
      function startTimer() {
        if (countdown) clearInterval(countdown);
        let remainingTime = time;
        sendCommand("start");
        countdown = setInterval(function () {
          remainingTime -= 1;
          document.getElementById("time").innerText = remainingTime.toFixed(2);
          // Cập nhật biểu đồ với giá trị tốc độ hiện tại
          monitoringChart.data.labels.push(time - remainingTime);
          monitoringChart.data.datasets[0].data.push(speed);
          monitoringChart.update();
          // Lưu trữ dữ liệu
          dataLogs.push({
            time: time - remainingTime,
            speed: speed,
            distance: distance,
          });
          if (remainingTime <= 0) {
            clearInterval(countdown);
            alert("Hoàn thành!");
          }
        }, 1000);
      }

      function stopTimer() {
        if (countdown) clearInterval(countdown);
        document.getElementById("time").innerText = time.toFixed(2);
        sendCommand("stop");
      }

      function resetSystem() {
        if (countdown) clearInterval(countdown);
        speed = 0;
        distance = 0;
        time = 0;
        acceleration = 0;
        updateDisplay();
        monitoringChart.data.labels = [];
        monitoringChart.data.datasets[0].data = [];
        monitoringChart.update();
        dataLogs = [];
        document.getElementById("dataLogs").innerHTML = "";
        sendCommand("reset");
      }

      // Gửi thông số (quãng đường và tốc độ) đến ESP8266 qua HTTP request
      function fetchParameters() {
        const distance = document.getElementById("distanceInput").value;
        const speed = document.getElementById("speedInput").value;
        if (distance && speed) {
          const url = `/control?distance=${distance}&speed=${speed}`;
          fetch(url)
            .then((response) => response.text())
            .then((data) => {
              console.log(data);
            })
            .catch((error) => {
              console.error("Lỗi khi gửi yêu cầu:", error);
            });
        } else {
          alert("Vui lòng nhập quãng đường và tốc độ hợp lệ.");
        }
      }

      // Gửi lệnh điều khiển đến ESP8266 (start, stop, reset, …)
      function sendCommand(cmd) {
        fetch("/control?command=" + cmd)
          .then((response) => response.text())
          .then((data) => alert(data))
          .catch((err) => alert("Lỗi: " + err));
      }

      // Xuất dữ liệu lưu trữ thành file JSON
      function exportData() {
        const blob = new Blob([JSON.stringify(dataLogs, null, 2)], {
          type: "application/json",
        });
        const url = URL.createObjectURL(blob);
        const a = document.createElement("a");
        a.href = url;
        a.download = "data_logs.json";
        a.click();
        URL.revokeObjectURL(url);
      }

      // -----------------------------
      // CHỨC NĂNG LẤY DỮ LIỆU TỪ ARDUINO (ESP8266 trả về JSON)
      // -----------------------------
      function updateChart(dataArray) {
        monitoringChart.data.labels = [];
        monitoringChart.data.datasets[0].data = [];
        dataArray.forEach((item) => {
          monitoringChart.data.labels.push(item.time);
          monitoringChart.data.datasets[0].data.push(item.speed);
        });
        monitoringChart.update();
      }

      function fetchData() {
        fetch("/data")
          .then((response) => response.json())
          .then((data) => {
            console.log("Dữ liệu nhận từ Arduino:", data);
            updateChart(data);
          })
          .catch((err) => console.error("Lỗi khi lấy dữ liệu:", err));
      }

      // Gọi hàm fetchData mỗi 3 giây để cập nhật biểu đồ
      setInterval(fetchData, 3000);
    </script>
  </body>
</html>
)rawliteral";

// Hàm xử lý trang chủ – gửi giao diện web
void handleRoot() {
  server.send_P(200, "text/html", htmlControl);
}

// Endpoint trả về dữ liệu JSON (nhận từ quá trình chuyển động)
void handleData() {
  server.send(200, "application/json", motorData);
}

// Endpoint để bắt đầu chuyển động (điều khiển động cơ)
void handleStart() {
  // Gọi hàm chuyển động (hàm này sẽ chạy blocking)
  runMotorMotion();
  // Sau đó trả về phản hồi
  server.send(200, "text/plain", "Operation completed");
}

// ====================
// SETUP CHÍNH
// ====================
void setup() {
  // --- Phần điều khiển động cơ và LCD ---
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  // Cài đặt ban đầu cho động cơ
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(motorAcceleration);
  
  // --- Serial dùng cho dữ liệu chuyển động ---  
  Serial.begin(9600);
  delay(1000);  // Đợi ổn định
  
  // --- Phần WiFi ---  
  // Chế độ Station + AP
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_sta, password_sta);
  Serial.println("Đang kết nối WiFi (STA)...");
  
  unsigned long startTime = millis();
  // Tăng timeout kết nối lên 30 giây (30000 ms)
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
    delay(500);
    Serial.print(".");
  }
  
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nĐã kết nối WiFi (STA)!");
    Serial.print("Địa chỉ IP (STA): ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nKhông kết nối được WiFi (STA).");
  }
  
  // Khởi tạo Access Point
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("Access Point đã sẵn sàng.");
  Serial.print("Địa chỉ IP (AP): ");
  Serial.println(WiFi.softAPIP());
  
  // --- Cài đặt Web Server ---
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/start", handleStart);
  server.begin();
  Serial.println("Server bắt đầu hoạt động...");
}


// ====================
// LOOP CHÍNH
// ====================
void loop() {
  // Chạy web server
  server.handleClient();
  
  // Đọc dữ liệu từ Serial (giả sử phần chuyển động đã in JSON)
  // Nếu có dòng bắt đầu bằng dấu [ thì cập nhật biến motorData
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    if (line.startsWith("[")) {
      motorData = line;
      Serial.println("Cập nhật motorData: " + motorData);
    }
  }
  
  // Nếu không có lệnh chuyển động nào, ta cho loop chạy nhẹ
  delay(10);
}
