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
  server.on("/control", handleControl);

  // Bắt đầu server
  server.begin();
  Serial.println("Server bắt đầu hoạt động...");
}

void loop() {
  server.handleClient(); // Xử lý yêu cầu từ client
}

// Hiển thị giao diện IoT
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
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
      const leadScrewPitch = 5; // Độ dày vít-me (mm/rev)
      const stepsPerRevolution = 200; // Số bước mỗi vòng (step/rev)
      const microstep = 16; // Vi bước

      let speed = 0; // Tốc độ (mm/s)
      let acceleration = 0; // Gia tốc (mm/s²)
      let distance = 0; // Quãng đường (mm)
      let time = 0; // Thời gian hoàn thành (s)
      let countdown; // Biến lưu trữ thời gian đếm ngược
      let dataLogs = []; // Lưu trữ dữ liệu
      let monitoringChart; // Biểu đồ giám sát

      // Khởi tạo biểu đồ
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
        acceleration = (speed * speed) / (2 * distance); // Gia tốc (mm/s²) = v² / (2 * d)
        time = distance / speed; // Thời gian (s)

        updateDisplay();
      }

      function updateDisplay() {
        document.getElementById("speed").innerText = speed.toFixed(2);
        document.getElementById("distance").innerText = distance.toFixed(2);
        document.getElementById("acceleration").innerText =
          acceleration.toFixed(2);
        document.getElementById("time").innerText = time.toFixed(2);
      }

      function setParameters() {
        calculateParameters();
        fetchParameters();
      }

      function startTimer() {
        if (countdown) clearInterval(countdown);
        let remainingTime = time;

        sendCommand("start");

        countdown = setInterval(function () {
          remainingTime -= 1;
          document.getElementById("time").innerText = remainingTime.toFixed(2);

          // Cập nhật biểu đồ
          monitoringChart.data.labels.push(time - remainingTime);
          monitoringChart.data.datasets[0].data.push(speed);
          monitoringChart.update();

          // Lưu dữ liệu
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

      function fetchParameters() {
        const distance = document.getElementById("distanceInput").value;
        const speed = document.getElementById("speedInput").value;

        if (distance && speed) {
          const url = `/control?distance=${distance}&speed=${speed}`;

          // Gửi dữ liệu tới ESP8266 thông qua HTTP request
          fetch(url)
            .then((response) => response.text())
            .then((data) => {
              console.log(data); // Hiển thị phản hồi từ ESP8266
            })
            .catch((error) => {
              console.error("Lỗi khi gửi yêu cầu:", error);
            });
        } else {
          alert("Vui lòng nhập quãng đường và tốc độ hợp lệ.");
        }
      }

      function sendCommand(cmd) {
        fetch("/control?command=" + cmd)
          .then((response) => response.text())
          .then((data) => alert(data))
          .catch((err) => alert("Lỗi: " + err));
      }

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
    </script>
  </body>
</html>



  )rawliteral";
  server.send(200, "text/html", html);
}

// Xử lý yêu cầu điều khiển
void handleControl() {
  if (server.hasArg("command")) {
    command = server.arg("command");
    Serial.println(command); // Gửi lệnh tới Arduino Uno
    server.send(200, "text/plain", "Đã gửi lệnh: " + command);
  } else if (server.hasArg("distance") && server.hasArg("speed")) {
    String distance = server.arg("distance");
    String speed = server.arg("speed");
    
    // Gửi dữ liệu tới Arduino qua cổng serial
    Serial.print("D:");  // Thêm ký hiệu để nhận biết dữ liệu
    Serial.print(distance);
    Serial.print(",");
    Serial.print("S:");
    Serial.print(speed);
    Serial.println();

    server.send(200, "text/plain", "Đã gửi lệnh: " + distance + "mm, " + speed + "mm/s");
  } else {
    server.send(400, "text/plain", "Lỗi: Không tìm thấy các tham số 'distance' và 'speed'.");
  }
}
