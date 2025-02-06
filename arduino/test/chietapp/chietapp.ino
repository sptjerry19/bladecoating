#include <AccelStepper.h>
#include <LiquidCrystal_I2C.h>

// Cấu hình LCD I2C (Địa chỉ 0x27, kích thước 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Định nghĩa chân cho driver động cơ bước (kiểu DRIVER: cần STEP và DIR)
#define STEP_PIN 8
#define DIR_PIN 9

// Khởi tạo đối tượng AccelStepper
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Thông số cơ học
const int stepsPerRevolution = 200;    // Bước/vòng của động cơ
const float leadScrewPitch = 5.0;        // Vít me di chuyển 5 mm mỗi vòng
const float distanceToMove = 100.0;      // 100 mm (10 cm)
const long totalSteps = (distanceToMove / leadScrewPitch) * stepsPerRevolution; // = 4000 bước

// Thời gian di chuyển: 10 giây cho hành trình tiến
const float timeToMove = 10.0;         

// Tính tốc độ mục tiêu (bước/giây) cần để di chuyển 10 cm trong 10 giây
const float targetSpeed = totalSteps / timeToMove; // 4000/10 = 400 bước/giây

void setup() {
  // Khởi tạo LCD
  lcd.init();
  lcd.backlight();
  
  // Hiển thị thông báo khởi động
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  
  // Cài đặt thông số cho AccelStepper:
  stepper.setMaxSpeed(targetSpeed);
  stepper.setAcceleration(targetSpeed / 2);  // Gia tốc, bạn có thể điều chỉnh thêm nếu cần
  
  // Khởi tạo Serial với tốc độ 9600 baud (đảm bảo khớp với ESP8266)
  Serial.begin(9600);
}

void loop() {
  // PHẦN 1: Di chuyển tiến 10 cm trong 10 giây
  lcd.setCursor(0, 0);
  lcd.print("Moving Forward ");
  
  // Đặt mục tiêu di chuyển về phía trước: +4000 bước
  stepper.moveTo(totalSteps);
  unsigned long startTime = millis();
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();  // Điều khiển với gia tốc và giảm tốc tự động

    // Gửi dữ liệu tốc độ qua Serial
    float currentSpeed = stepper.speed();
    Serial.print("Speed:");
    Serial.println(currentSpeed, 1);  // ví dụ: Speed:400.0
    
    // Cập nhật hiển thị LCD (hiển thị tốc độ hiện tại)
    lcd.setCursor(0, 1);
    lcd.print("Spd:");
    lcd.print(currentSpeed, 1);
    lcd.print(" steps/s   ");
  }
  
  // Nếu chuyển động hoàn thành chưa đủ 10 giây thì đợi thêm
  unsigned long elapsed = millis() - startTime;
  if (elapsed < 10000) {
    delay(10000 - elapsed);
  }
  
  // PHẦN 2: Quay lại vị trí ban đầu (10 cm ngược lại)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Returning Back");
  
  // Đặt mục tiêu trở về vị trí ban đầu: 0 bước
  stepper.moveTo(0);
  startTime = millis();
  
  while (stepper.distanceToGo() != 0) {
    stepper.run();
    
    float currentSpeed = stepper.speed();
    Serial.print("Speed:");
    Serial.println(currentSpeed, 1);
    
    lcd.setCursor(0, 1);
    lcd.print("Spd:");
    lcd.print(currentSpeed, 1);
    lcd.print(" steps/s   ");
  }
  
  elapsed = millis() - startTime;
  if (elapsed < 10000) {
    delay(10000 - elapsed);
  }
  
  // Kết thúc hoạt động
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Operation Done!");
  
  while (true); // Dừng chương trình
}
