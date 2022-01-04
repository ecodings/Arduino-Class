int Tact = 2;  //Tact 스위치 핀 번호 입력
int mode, check;

void setup() {
  pinMode(Tact, INPUT);
  mode = 0;
  check=0;
  analogWrite(9,0);
  analogWrite(10,0);
  analogWrite(11,0);
}

void loop() {
  if (digitalRead(Tact) == HIGH && check==0) {  //스위치를 눌렀을 때
    mode=(mode+1)%4;   //mode에 1을 증가시킨 후 4로 나눈 나머지.
    check=1;
  }
  if (digitalRead(Tact) == LOW && check==1) {  
    if (mode == 1) { //빨간색을 켜주라는 1번 상태입니다.
      analogWrite(9,0); 
      analogWrite(10,255);
      analogWrite(11,255);
    }
    if (mode == 2) { //녹색을 켜주라는 2번 상태입니다.
 
     
     
     }
    if (mode == 3) { //파란색을 켜주라는 3번 상태입니다.
 
     
     
     }
    if (mode == 0) { //모두 끄라는 0번 상태입니다.
  
        
        
        }
    check=0;
  }
}
