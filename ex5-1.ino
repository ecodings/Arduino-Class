int Tact = 2;
int check, n;

void setup() {
    pinMode(Tact, INPUT);
    check=0;
    n=0;
    Serial.begin(9600);
}

void loop() {
    if (digitalRead(Tact) == HIGH && check==0) { //스위치를 눌렀을 때
       n+=1;
       check=1;
       Serial.println(n);
    }

    if (digitalRead(Tact) == LOW && check==1) {
       check=0;
    }
}
