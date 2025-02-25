#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// --- Thông tin WiFi ---
// Chế độ Station (kết nối mạng hiện có)
const char* ssid_sta = "cheese";
const char* password_sta = "hoilamgi";
// Chế độ Access Point (tự phát WiFi)
const char* ssid_ap = "blade_coating";
const char* password_ap = "12345678";

// Khởi tạo WebServer chạy trên cổng 80
ESP8266WebServer server(80);

// Biến lưu trữ lệnh điều khiển (nếu cần)
String command = "";
// Biến lưu trữ dữ liệu JSON nhận từ Arduino (mặc định là mảng rỗng)
String motorData = "[]";

// --- Giao diện Web ---
// Lưu ý: Toàn bộ nội dung HTML/JavaScript được bao bọc trong raw literal string
const char htmlControl[] PROGMEM = R"rawliteral(
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
        text-align: left;
      }
      .input-group label {
        display: block;
        margin-bottom: 5px;
        font-size: 16px;
        font-weight: bold;
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
      .radio-group {
        display: flex;
        flex-direction: column;
        gap: 8px; /* Khoảng cách giữa các lựa chọn */
      }

      .radio-group label {
        display: flex;
        align-items: center;
        gap: 8px; /* Khoảng cách giữa input và chữ */
      }

      .radio-group input {
        margin: 0;
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

      <!-- Nhập quãng đường -->
      <div class="input-group">
        <label for="distanceInput">Nhập quãng đường (mm):</label>
        <input
          type="number"
          id="distanceInput"
          min="0"
          placeholder="Nhập quãng đường..."
        />
      </div>

      <!-- Chọn chế độ tính: theo tốc độ hoặc theo thời gian -->
      <div class="radio-group">
        <label>
          <input
            type="radio"
            name="mode"
            value="speed"
            checked
            onchange="toggleMode(this.value)"
          />
          Tính theo tốc độ
        </label>
        <label>
          <input
            type="radio"
            name="mode"
            value="time"
            onchange="toggleMode(this.value)"
          />
          Tính theo thời gian hoàn thành
        </label>
      </div>

      <!-- Nhập tốc độ (mm/s) -->
      <div class="input-group" id="speedInputContainer">
        <label for="speedInput">Nhập tốc độ (mm/s):</label>
        <input
          type="number"
          id="speedInput"
          min="0"
          placeholder="Nhập tốc độ..."
        />
      </div>

      <!-- Nhập thời gian hoàn thành (s) -->
      <div class="input-group" id="timeInputContainer" style="display: none">
        <label for="timeInput">Nhập thời gian hoàn thành (s):</label>
        <input
          type="number"
          id="timeInput"
          min="0"
          placeholder="Nhập thời gian hoàn thành..."
        />
      </div>

      <div class="button-group">
        <button onclick="backward()">Lùi</button>
        <button onclick="setParameters()">Cập nhật thông số</button>
        <button onclick="forward()">Tiến</button>
      </div>

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
        <canvas id="monitoringChart" width="500" height="300"></canvas>
      </div>

      <!-- Lưu trữ dữ liệu -->
      <div class="data-container">
        <h2>Lưu trữ dữ liệu</h2>
        <button onclick="exportData()">Xuất dữ liệu</button>
        <div id="dataLogs"></div>
      </div>
    </div>

    <script>
      // Thông số vít-mê
      const leadScrewPitch = 5; // mm/rev
      const stepsPerRevolution = 200; // step/rev
      const microstep = 16; // vi bước

      let speed = 0; // mm/s
      let acceleration = 0; // mm/s²
      let distance = 0; // mm
      let time = 0; // giây
      let countdown; // biến đếm ngược
      let dataLogs = []; // lưu trữ dữ liệu

      // Dữ liệu cho biểu đồ
      let chartData = {
        times: [],
        speeds: [],
      };

      // Biến lưu chế độ hiện tại: "speed" hoặc "time"
      let selectedMode = "speed";

      // Hàm chuyển đổi giữa các chế độ nhập
      function toggleMode(mode) {
        selectedMode = mode;
        if (mode === "speed") {
          document.getElementById("speedInputContainer").style.display =
            "block";
          document.getElementById("timeInputContainer").style.display = "none";
        } else {
          document.getElementById("speedInputContainer").style.display = "none";
          document.getElementById("timeInputContainer").style.display = "block";
        }
      }

      // Tính toán thông số động cơ dựa trên input và chế độ được chọn
      function calculateParameters() {
        const inputDistance = parseFloat(
          document.getElementById("distanceInput").value
        );
        if (isNaN(inputDistance) || inputDistance <= 0) {
          alert("Vui lòng nhập quãng đường hợp lệ.");
          return false;
        }
        distance = inputDistance;
        if (selectedMode === "speed") {
          const inputSpeed = parseFloat(
            document.getElementById("speedInput").value
          );
          if (isNaN(inputSpeed) || inputSpeed <= 0) {
            alert("Vui lòng nhập tốc độ hợp lệ.");
            return false;
          }
          speed = inputSpeed;
          time = distance / speed;
        } else {
          const inputTime = parseFloat(
            document.getElementById("timeInput").value
          );
          if (isNaN(inputTime) || inputTime <= 0) {
            alert("Vui lòng nhập thời gian hoàn thành hợp lệ.");
            return false;
          }
          time = inputTime;
          speed = distance / time;
        }
        acceleration = (speed * speed) / (2 * distance);
        updateDisplay();
        return true;
      }

      // Cập nhật thông số hiển thị trên trang
      function updateDisplay() {
        document.getElementById("speed").innerText = speed.toFixed(2);
        document.getElementById("distance").innerText = distance.toFixed(2);
        document.getElementById("acceleration").innerText =
          acceleration.toFixed(2);
        document.getElementById("time").innerText = time.toFixed(2);
      }

      // Gọi tính toán và gửi thông số đến ESP8266 (giả lập)
      function setParameters() {
        if (calculateParameters()) {
          fetchParameters();
        }
      }

      // Vẽ biểu đồ với các giá trị trên trục x và y
      function drawChart() {
        const canvas = document.getElementById("monitoringChart");
        if (!canvas.getContext) return;
        const ctx = canvas.getContext("2d");
        const width = canvas.width;
        const height = canvas.height;
        // Xóa canvas
        ctx.clearRect(0, 0, width, height);

        // Định nghĩa lề cho biểu đồ
        const margin = { top: 20, right: 40, bottom: 40, left: 50 };
        const chartWidth = width - margin.left - margin.right;
        const chartHeight = height - margin.top - margin.bottom;

        // Vẽ trục y và trục x
        ctx.strokeStyle = "#333";
        ctx.lineWidth = 1;
        // Trục y
        ctx.beginPath();
        ctx.moveTo(margin.left, margin.top);
        ctx.lineTo(margin.left, margin.top + chartHeight);
        ctx.stroke();
        // Trục x
        ctx.beginPath();
        ctx.moveTo(margin.left, margin.top + chartHeight);
        ctx.lineTo(margin.left + chartWidth, margin.top + chartHeight);
        ctx.stroke();

        // --- Vẽ các dấu tick và nhãn trên trục y (tốc độ) ---
        const maxSpeed = Math.max(...chartData.speeds, speed * 2);
        const yTickCount = 5;
        ctx.fillStyle = "#333";
        ctx.font = "12px Arial";
        for (let i = 0; i <= yTickCount; i++) {
          const yVal = (maxSpeed / yTickCount) * i;
          const yPos =
            margin.top + chartHeight - (yVal / maxSpeed) * chartHeight;
          // Vẽ dấu tick bên trái trục
          ctx.beginPath();
          ctx.moveTo(margin.left - 5, yPos);
          ctx.lineTo(margin.left, yPos);
          ctx.stroke();
          // Vẽ đường lưới ngang (tùy chọn)
          ctx.strokeStyle = "#ccc";
          ctx.beginPath();
          ctx.moveTo(margin.left, yPos);
          ctx.lineTo(margin.left + chartWidth, yPos);
          ctx.stroke();
          ctx.strokeStyle = "#333";
          // Vẽ nhãn
          ctx.textAlign = "right";
          ctx.textBaseline = "middle";
          ctx.fillText(yVal.toFixed(1), margin.left - 8, yPos);
        }

        // --- Vẽ các dấu tick và nhãn trên trục x (thời gian) ---
        const maxTime =
          time > 0
            ? time
            : chartData.times.length > 0
            ? chartData.times[chartData.times.length - 1]
            : 1;
        const xTickCount = 5;
        for (let i = 0; i <= xTickCount; i++) {
          const xVal = (maxTime / xTickCount) * i;
          const xPos = margin.left + (chartWidth / xTickCount) * i;
          // Vẽ dấu tick bên dưới trục
          ctx.beginPath();
          ctx.moveTo(xPos, margin.top + chartHeight);
          ctx.lineTo(xPos, margin.top + chartHeight + 5);
          ctx.stroke();
          // Vẽ đường lưới dọc (tùy chọn)
          ctx.strokeStyle = "#ccc";
          ctx.beginPath();
          ctx.moveTo(xPos, margin.top);
          ctx.lineTo(xPos, margin.top + chartHeight);
          ctx.stroke();
          ctx.strokeStyle = "#333";
          // Vẽ nhãn
          ctx.textAlign = "center";
          ctx.textBaseline = "top";
          ctx.fillText(xVal.toFixed(1), xPos, margin.top + chartHeight + 8);
        }

        // --- Vẽ đường biểu đồ nếu có đủ dữ liệu ---
        if (chartData.times.length < 2) return;
        ctx.beginPath();
        const n = chartData.times.length;
        for (let i = 0; i < n; i++) {
          // Vị trí x theo tỷ lệ thời gian
          const x = margin.left + (chartData.times[i] / maxTime) * chartWidth;
          // Vị trí y theo tỷ lệ tốc độ
          const y =
            margin.top +
            chartHeight -
            (chartData.speeds[i] / maxSpeed) * chartHeight;
          if (i === 0) {
            ctx.moveTo(x, y);
          } else {
            ctx.lineTo(x, y);
          }
        }
        ctx.strokeStyle = "#3498db";
        ctx.lineWidth = 2;
        ctx.stroke();

        // Vẽ các điểm dữ liệu
        for (let i = 0; i < n; i++) {
          const x = margin.left + (chartData.times[i] / maxTime) * chartWidth;
          const y =
            margin.top +
            chartHeight -
            (chartData.speeds[i] / maxSpeed) * chartHeight;
          ctx.beginPath();
          ctx.arc(x, y, 3, 0, 2 * Math.PI);
          ctx.fillStyle = "#3498db";
          ctx.fill();
        }
      }

      // Hàm gửi dữ liệu đến API POST sau khi chạy xong
      function postDataToServer() {
        // Chuẩn bị body của POST request theo cấu trúc yêu cầu
        const postBody = {
          data: dataLogs, // dataLogs là mảng chứa dữ liệu các mẫu đã thu thập
        };

        fetch("https://aahome.click/api/v1/motor", {
          method: "POST",
          headers: {
            "Content-Type": "application/json",
          },
          body: JSON.stringify(postBody),
        })
          .then((response) => response.json())
          .then((result) => {
            console.log("API response:", result);
            alert("Gửi dữ liệu lên server thành công!");
          })
          .catch((error) => {
            console.error("Error posting data:", error);
            alert("Gửi dữ liệu gặp lỗi!");
          });
      }

      // gửi chuyển động "tiến"
      function forward() {
        sendCommand("forward");
      }

      // gửi chuyển động "lùi"
      function backward() {
        sendCommand("backward");
      }

      // Bắt đầu chuyển động và cập nhật biểu đồ theo thời gian
      function startTimer() {
        if (countdown) clearInterval(countdown);
        let remainingTime = time;
        sendCommand("start");
        countdown = setInterval(function () {
          remainingTime -= 1;
          document.getElementById("time").innerText = remainingTime.toFixed(2);
          // Cập nhật dữ liệu cho biểu đồ
          chartData.times.push(time - remainingTime);
          chartData.speeds.push(speed);
          drawChart();
          // Lưu trữ dữ liệu
          dataLogs.push({
            time: time - remainingTime,
            speed: speed,
            distance: distance,
          });
          if (remainingTime <= 0) {
            clearInterval(countdown);
            alert("Hoàn thành!");

            // Sau khi hoàn thành, gọi API POST để gửi dữ liệu lên server
            postDataToServer();
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
        chartData.times = [];
        chartData.speeds = [];
        drawChart();
        dataLogs = [];
        document.getElementById("dataLogs").innerHTML = "";
        sendCommand("reset");
      }

      // Gửi thông số đến ESP8266 (giả lập)
      function fetchParameters() {
        const distanceValue = document.getElementById("distanceInput").value;
        // Lưu ý: tốc độ hoặc thời gian đã được tính trong calculateParameters()
        if (distanceValue) {
          const url = `/control?distance=${distanceValue}&speed=${speed}`;
          fetch(url)
            .then((response) => response.text())
            .then((data) => {
              console.log(data);
            })
            .catch((error) => {
              console.error("Lỗi khi gửi yêu cầu:", error);
            });
        } else {
          alert("Vui lòng nhập quãng đường hợp lệ.");
        }
      }

      // Gửi lệnh điều khiển đến ESP8266 (start, stop, reset,...)
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
      function updateChartWithData(dataArray) {
        // Giả sử dataArray là mảng đối tượng có thuộc tính time và speed
        chartData.times = [];
        chartData.speeds = [];
        dataArray.forEach((item) => {
          chartData.times.push(item.time);
          chartData.speeds.push(item.speed);
        });
        drawChart();
      }

      function fetchData() {
        fetch("/data")
          .then((response) => response.json())
          .then((data) => {
            console.log("Dữ liệu nhận từ Arduino:", data);
            updateChartWithData(data);
          })
          .catch((err) => console.error("Lỗi khi lấy dữ liệu:", err));
      }

      // Gọi hàm fetchData mỗi 3 giây để cập nhật biểu đồ
      setInterval(fetchData, 3000);
    </script>
  </body>
</html>



)rawliteral";

void handleRoot() {
  server.send_P(200, "text/html", htmlControl);
}

// Xử lý yêu cầu điều khiển từ client
void handleControl() {
  if (server.hasArg("command")) {
    command = server.arg("command");
    Serial.println(command); // Gửi lệnh qua Serial tới Arduino
    server.send(200, "text/plain", "Đã gửi lệnh: " + command);
  } else if (server.hasArg("distance") && server.hasArg("speed")) {
    String distance = server.arg("distance");
    String speed = server.arg("speed");
    // Gửi dữ liệu tới Arduino qua Serial với định dạng đặc biệt
    Serial.print("D:");
    Serial.print(distance);
    Serial.print(",S:");
    Serial.print(speed);
    Serial.println();
    server.send(200, "text/plain", "Đã gửi lệnh: " + distance + "mm, " + speed + "mm/s");
  } else {
    server.send(400, "text/plain", "Lỗi: Không tìm thấy các tham số 'distance' và 'speed'.");
  }
}

// Xử lý yêu cầu lấy dữ liệu JSON từ Arduino
void handleData() {
  server.send(200, "application/json", motorData);
}

void setup() {
  // --- Phần WiFi ---
  Serial.begin(9600);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid_sta, password_sta);
  Serial.println("Đang kết nối WiFi (STA)...");
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
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
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("Access Point đã sẵn sàng.");
  Serial.print("Địa chỉ IP (AP): ");
  Serial.println(WiFi.softAPIP());
  
  // --- Cài đặt Web Server ---
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Server bắt đầu hoạt động...");
}

void loop() {
  server.handleClient();
  
  // Đọc dữ liệu từ Arduino qua Serial
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("[")) {
      motorData = line;
      Serial.println("Cập nhật motorData: " + motorData);
    }
  }
  
  delay(10);
}
