const mockData = {
  speed: (Math.random() * 100).toFixed(1),
  comparison: (Math.random() * 20000).toFixed(0),
};

function updateMockData() {
  mockData.speed = (Math.random() * 100).toFixed(1);
  mockData.comparison = (Math.random() * 20000).toFixed(0);
}

// Quãng đường cố định (20 cm)
const distance = 20;

// Hàm tính toán thời gian còn lại
function calculateRemainingTime() {
  const speed = parseFloat(mockData.speed); // Lấy tốc độ từ mockData và chuyển thành số
  const remainingTime = (distance / speed).toFixed(2); // Tính thời gian còn lại (giây)
  const percentageChange = (Math.random() * 10).toFixed(1); // Phần trăm thay đổi ngẫu nhiên
  const comparisonWeek = Math.floor(Math.random() * 30); // Giá trị so sánh tuần trước ngẫu nhiên

  // Cập nhật nội dung HTML với thời gian còn lại
  document.getElementById(
    "remaining-time"
  ).textContent = `${remainingTime} seconds`;
  document.getElementById(
    "percentage-time"
  ).textContent = `${percentageChange}%`;
  document.getElementById(
    "comparison-week-time"
  ).textContent = `Compared to last week ${comparisonWeek} seconds`;
  document.getElementById("speed-value").textContent = `${mockData.speed} m/h`;
  document.getElementById(
    "comparison-text"
  ).textContent = `Tốc độ tối đa: ${mockData.comparison} m/h`;
}

// Cập nhật mockData và thời gian còn lại mỗi 2 giây
setInterval(() => {
  updateMockData(); // Cập nhật mockData với giá trị ngẫu nhiên mới
  calculateRemainingTime(); // Tính toán và hiển thị thời gian còn lại
}, 2000);

// Gọi lần đầu khi trang được tải
window.onload = () => {
  updateMockData();
  calculateRemainingTime();
};

function updateTemperatureData() {
  // Tạo giá trị ngẫu nhiên cho nhiệt độ và phần trăm thay đổi
  const temperatureValue = (Math.random() * 40).toFixed(1); // Nhiệt độ ngẫu nhiên từ 0 - 40 độ C
  const percentageChange = (Math.random() * 10).toFixed(1); // Phần trăm thay đổi ngẫu nhiên từ 0 - 10%
  const comparisonWeek = Math.floor(Math.random() * 30); // Giá trị ngẫu nhiên cho so sánh tuần trước (0 - 30)

  // Cập nhật nội dung HTML với dữ liệu ngẫu nhiên
  document.getElementById(
    "temperature-value"
  ).textContent = `${temperatureValue}°C`;
  document.getElementById(
    "percentage-change"
  ).textContent = `${percentageChange}%`;
  document.getElementById(
    "comparison-week"
  ).textContent = `Compared to last week ${comparisonWeek}`;
}

// Gọi updateTemperatureData mỗi 2 giây
setInterval(updateTemperatureData, 2000);

// Gọi updateTemperatureData khi trang tải lần đầu
window.onload = updateTemperatureData;
