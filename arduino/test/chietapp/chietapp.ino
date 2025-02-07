#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

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
const float distanceToMove = 200.0;      // Di chuyển 200 mm (thay vì 100 mm)
// Tính tổng số bước cần thiết
const long totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution;

// Thời gian di chuyển sẽ được nhập từ người dùng (đơn vị: giây)
float timeToMove = 0.0;         

// Tốc độ mục tiêu (bước/giây) được tính dựa trên timeToMove
float targetSpeed = 0.0;

// Chuỗi lưu trữ dữ liệu JSON (dùng để lưu lại mẫu dữ liệu trong quá trình di chuyển)
String jsonData = "";
bool firstSample = true;
int sampleCounter = 0;

// Hàm ghi nhận mẫu dữ liệu (số mẫu tăng dần, tốc độ và quãng đường hiện tại)
void recordSample() {
  sampleCounter++;
  // Lấy tốc độ hiện tại (bước/giây) và đổi sang mm/s:
  float currentSpeedSteps = stepper.speed();  
  float currentSpeedMM = currentSpeedSteps * leadScrewPitch / stepsPerRevolution;
  
  // Lấy quãng đường đã di chuyển (mm)
  float currentDistanceMM = abs(stepper.currentPosition()) * leadScrewPitch / stepsPerRevolution;
  
  // Nếu không phải mẫu đầu tiên, thêm dấu phẩy và xuống dòng
  if (!firstSample) {
    jsonData += ",\n";
  }
  
  jsonData += "  { \"time\": " + String(sampleCounter) +
              ", \"speed\": " + String(currentSpeedMM, 1) +
              ", \"distance\": " + String(currentDistanceMM, 1) + " }";
  
  firstSample = false;
}

// Hàm kiểm tra và xử lý dữ liệu JSON nhận được qua Serial từ ESP8266
void checkForIncomingJSON() {
  while (Serial.available() > 0) {
    String incoming = Serial.readStringUntil('\n');
    incoming.trim();
    // Kiểm tra xem chuỗi nhận được có dạng JSON không (bắt đầu { và kết thúc })
    if (incoming.startsWith("{") && incoming.endsWith("}")) {
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, incoming);
      if (!error) {
        const char* command = doc["command"];
        if (command != NULL) {
          Serial.print("Received command: ");
          Serial.println(command);
          // Ví dụ: lệnh "setTime" để cập nhật thời gian di chuyển mới
          if (strcmp(command, "setTime") == 0) {
            float newTime = doc["time"];
            if (newTime > 0) {
              timeToMove = newTime;
              targetSpeed = totalSteps / timeToMove;
              stepper.setMaxSpeed(targetSpeed);
              stepper.setAcceleration(targetSpeed / 2);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("New time: ");
              lcd.print(timeToMove);
              Serial.println("Time updated via JSON");
            }
          }
          // Các lệnh khác (start, stop, …) có thể được xử lý tại đây.
          else if (strcmp(command, "start") == 0) {
            Serial.println("Start command received");
            // Bạn có thể cài đặt flag để bắt đầu chuyển động nếu cần.
          }
          else if (strcmp(command, "stop") == 0) {
            Serial.println("Stop command received");
            // Thực hiện dừng chuyển động nếu cần.
          }
        }
      } else {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
      }
    }
  }
}

// Hàm di chuyển với lấy mẫu và kiểm tra nhận JSON trong quá trình chuyển động
void moveWithSampling(long target, unsigned long moveDuration) {
  stepper.moveTo(target);
  unsigned long startTime = millis();
  unsigned long lastSampleTime = millis();
  
  // Chạy cho đến khi đạt mục tiêu
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    // Trong quá trình chạy, kiểm tra nếu có JSON gửi đến
    checkForIncomingJSON();
    
    // Nếu đã hơn 1 giây thì lấy mẫu dữ liệu
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

void setup() {
  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  
  // Khởi tạo Serial (9600 baud)
  Serial.begin(9600);
  
  // Hiển thị thông báo khởi động trên LCD
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  // Yêu cầu người dùng nhập thời gian di chuyển qua Serial
  Serial.println("Nhap thoi gian di chuyen (giay): ");
  lcd.setCursor(0, 0);
  lcd.print("Nhap thoi gian:");
  
  // Chờ nhận dữ liệu nhập từ Serial
  while (Serial.available() == 0) {
    // Đợi nhập
  }
  String timeInput = Serial.readStringUntil('\n');
  timeToMove = timeInput.toFloat();
  if (timeToMove <= 0) {
    timeToMove = 10.0;  // Nếu dữ liệu không hợp lệ, sử dụng giá trị mặc định 10 giây
    Serial.println("Gia tri nhap khong hop le, su dung mac dinh 10 giay.");
  } else {
    Serial.print("Thoi gian di chuyen: ");
    Serial.print(timeToMove);
    Serial.println(" giay");
  }
  
  // Tính tốc độ mục tiêu (bước/giây) và cài đặt cho stepper
  targetSpeed = totalSteps / timeToMove;
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);
  
  // Bắt đầu chuỗi JSON
  jsonData = "[\n";
}

void loop() {
  // PHẦN 1: Di chuyển tiến (200 mm) trong khoảng thời gian timeToMove
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Moving Forward");
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
  
  // Gửi dữ liệu JSON qua Serial (để ESP8266 hoặc thiết bị nhận khác đọc)
  Serial.println(jsonData);
  
  // Sau khi gửi dữ liệu, tiếp tục kiểm tra và nhận lệnh JSON (nếu có) và dừng chương trình
  while (true) {
    checkForIncomingJSON();
  }
}
