const int stepPin = 8; //PUL
const int dirPin = 9; // DIR
const int enPin = 10; //ena
void setup() {
  
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  pinMode(enPin,OUTPUT);
  digitalWrite(enPin,LOW);
  
}
void loop() {
  
  digitalWrite(dirPin,HIGH); // cho đông cơ quay theo chiều thuận
  
  for(int x = 0; x < 200; x++) { // CHẠY 1 VÒNG 200 STEP / 1.8 ĐỘ = 360, S1 S2 ON, S3 OFF
    digitalWrite(stepPin,HIGH); 
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(500); 
  }
  delay(1000); 

  digitalWrite(dirPin,LOW); //thay đổi chiều quay
  
  for(int x = 0; x < 200; x++) {
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
  }
  delay(1000);
  
}
