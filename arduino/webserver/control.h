String htmlControl = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Điều khiển động cơ bước & Giám sát</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background-color: #f4f4f9;
        margin: 0;
        padding: 20px;
        text-align: center;
      }
      .chart-container {
        margin: 20px auto;
        max-width: 600px;
      }
    </style>
  </head>
  <body>
    <h1>Giám sát động cơ bước</h1>
    <!-- Biểu đồ giám sát -->
    <div class="chart-container">
      <h2>Biểu đồ giám sát</h2>
      <canvas id="monitoringChart" width="400" height="200"></canvas>
    </div>
    
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
      // Khởi tạo biểu đồ
      const ctx = document.getElementById("monitoringChart").getContext("2d");
      const monitoringChart = new Chart(ctx, {
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
              title: { display: true, text: "Thời gian (s)" },
            },
            y: {
              title: { display: true, text: "Tốc độ (mm/s)" },
            },
          },
        },
      });
      
      // Hàm cập nhật biểu đồ với dữ liệu từ Arduino (lấy từ ESP8266)
      function updateChart(dataArray) {
        // dataArray là mảng các đối tượng có key: time, speed, distance
        monitoringChart.data.labels = [];
        monitoringChart.data.datasets[0].data = [];
        
        dataArray.forEach(item => {
          monitoringChart.data.labels.push(item.time);
          monitoringChart.data.datasets[0].data.push(item.speed);
        });
        
        monitoringChart.update();
      }
      
      // Ví dụ: lấy dữ liệu từ endpoint /data trên ESP8266
      function fetchData() {
        fetch('/data')
          .then(response => response.json())
          .then(data => {
            console.log("Nhận dữ liệu:", data);
            updateChart(data);
          })
          .catch(err => console.error("Lỗi khi lấy dữ liệu:", err));
      }
      
      // Gọi fetchData sau khi trang được tải
      window.onload = fetchData;
    </script>
  </body>
</html>

)rawliteral";