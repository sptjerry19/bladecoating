#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>

// Cấu hình LCD I2C (Địa chỉ 0x27, kích thước 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Định nghĩa chân cho driver động cơ bước (kiểu DRIVER: cần STEP và DIR)
#define STEP_PIN 8
#define DIR_PIN 9

// Khởi tạo đối tượng AccelStepper (chế độ DRIVER)
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Thông số cơ học mới
const int stepsPerRevolution = 6400;   // Bước/vòng của động cơ (đã thay đổi)
const float leadScrewPitch = 5.0;        // Vít me di chuyển 5 mm mỗi vòng
const float distanceToMove = 200.0;      // 200 mm (thay vì 100 mm)
// Tính tổng số bước cần thiết
const long totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;

// Thời gian di chuyển sẽ được nhập từ người dùng (đơn vị: giây)
float timeToMove = 0.0;         

// Tốc độ mục tiêu (bước/giây) sẽ được tính lại sau khi có timeToMove
float targetSpeed = 0.0;

// Chuỗi lưu trữ dữ liệu JSON
String jsonData = "";
bool firstSample = true;
int sampleCounter = 0;

void setup() {
  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  
  // Khởi tạo Serial với tốc độ 9600 baud
  Serial.begin(9600);
  
  // Hiển thị thông báo khởi động trên LCD
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  // Yêu cầu người dùng nhập thời gian di chuyển
  Serial.println("Nhap thoi gian di chuyen (giay): ");
  lcd.setCursor(0, 0);
  lcd.print("Nhap thoi gian:");
  
  // Chờ người dùng nhập dữ liệu qua Serial
  while (Serial.available() == 0) {
    // Đợi nhập dữ liệu
  }
  // Đọc chuỗi nhập vào và chuyển sang float
  String timeInput = Serial.readStringUntil('\n');
  timeToMove = timeInput.toFloat();
  if(timeToMove <= 0) {
    timeToMove = 10.0;  // Nếu dữ liệu không hợp lệ, sử dụng giá trị mặc định 10 giây
    Serial.println("Gia tri nhap khong hop le, su dung mac dinh 10 giay.");
  } else {
    Serial.print("Thoi gian di chuyen: ");
    Serial.print(timeToMove);
    Serial.println(" giay");
  }
  
  // Tính tốc độ mục tiêu (bước/giây)
  targetSpeed = totalSteps / timeToMove;
  
  // Cài đặt thông số cho AccelStepper
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);  // Gia tốc (có thể điều chỉnh)
  
  // Bắt đầu chuỗi JSON
  jsonData = "[\n";
}

void recordSample() {
  sampleCounter++;
  // Lấy tốc độ hiện tại (steps/s) và đổi sang mm/s:
  float currentSpeedSteps = stepper.speed();  // tốc độ hiện tại (bước/giây)
  float currentSpeedMM = currentSpeedSteps * leadScrewPitch / stepsPerRevolution;
  
  // Lấy quãng đường di chuyển được (tính theo mm)
  float currentDistanceMM = abs(stepper.currentPosition()) * leadScrewPitch / stepsPerRevolution;
  
  // Nếu không phải mẫu đầu tiên thì thêm dấu phẩy
  if (!firstSample) {
    jsonData += ",\n";
  }
  
  jsonData += "  { \"time\": " + String(sampleCounter) +
              ", \"speed\": " + String(currentSpeedMM, 1) +
              ", \"distance\": " + String(currentDistanceMM, 1) + " }";
  
  firstSample = false;
}

void moveWithSampling(long target, unsigned long moveDuration) {
  // Đặt mục tiêu chuyển động
  stepper.moveTo(target);
  unsigned long startTime = millis();
  unsigned long lastSampleTime = millis();
  
  // Chạy cho đến khi đạt mục tiêu
  while (stepper.distanceToGo() != 0) {
    stepper.run();  // Chạy với gia tốc giảm tốc tự động
    
    // Nếu đã hơn 1 giây thì lấy mẫu
    if (millis() - lastSampleTime >= 1000) {
      recordSample();
      lastSampleTime = millis();
    }
  }
  
  // Nếu thời gian chuyển động chưa đủ moveDuration, đợi thêm
  unsigned long elapsed = millis() - startTime;
  if (elapsed < moveDuration) {
    delay(moveDuration - elapsed);
  }
}

void loop() {
  // PHẦN 1: Di chuyển tiến (200 mm) trong thời gian timeToMove (đã nhập từ người dùng)
  lcd.setCursor(0, 0);
  lcd.print("Moving Forward ");
  moveWithSampling(totalSteps, (unsigned long)(timeToMove * 1000));
  
  // PHẦN 2: Di chuyển lùi về vị trí ban đầu
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Returning Back");
  moveWithSampling(0, (unsigned long)(timeToMove * 1000));
  
  // Kết thúc hoạt động
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Operation Done!");
  
  // Kết thúc chuỗi JSON
  jsonData += "\n]";
  
  // Gửi JSON qua Serial
  Serial.println(jsonData);
  
  // Dừng chương trình
  while (true);
}
