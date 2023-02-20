#define SERIAL_SPEED 115200
#define OBJ_FIND_DISTANCE_CM 7
#define PIN_ECHO D2 
#define PIN_TRIG D4
#define MOT_1_+_PIN D10
#define MOT_1_-_PIN D9
#define MOT_2_+_PIN D6
#define MOT_2_-_PIN D5
#define WAITING_ST 0
#define RUNNING_ST 1
#define SCANNING_ST 2
#define SCAN_SUCCESS_ST 3
#define SCAN_ERROR_ST 4

int state = WAITING_ST; // S0
bool delayedStop = false;
uint32_t myTimer1
//////////////////////////////////////////////////////////////////////////////
void conveyor_stop(){// первая лента остановка
	digitalWrite(MOT_1_+_PIN, 0);// M3
	digitalWrite(MOT_1_-_PIN, 0);
}
void conveyor_stop_2(){// вторая лента остановка
	digitalWrite(MOT_2_+_PIN, 0);// M4
	digitalWrite(MOT_2_-_PIN, 0);
}
void conveyor_run(){//первая лента запуск
	digitalWrite(MOT_1_+_PIN, 1);
	digitalWrite(MOT_1_-_PIN, 0);
}
void conveyor_run_2_L(){// вторая лента запуск влево 
	digitalWrite(MOT_2_+_PIN, 1);
	digitalWrite(MOT_2_-_PIN, 0);
	if (millis() - myTimer1 >= 6000) {  
		myTimer1 = millis();
		return;
	}
}
void conveyor_run_2_R(){// второая лента запуск вправо
	digitalWrite(MOT_2_+_PIN, 0);
	digitalWrite(MOT_2_-_PIN, 1);
	if (millis() - myTimer1 >= 6000) {   
		myTimer1 = millis();
		return;
	}
}
//////////////////////////////////////////////////////////////////////////////////
void setup(){
	Serial.begin(SERIAL_SPEED);
	pinMode(PIN_TRIG, OUTPUT);
	pinmode(PIN_ECHO, INPUT);
	pinMode(MOT_1_+_PIN, OUTPUT);
	pinmode(MOT_2_+_PIN, OUTPUT);
	pinmode(MOT_2_-_PIN, OUTPUT);
}
//////////////////////////////////////////////////////////////////////////////////
void waitingSt_handler(int cmd){
	switch(cmd){
		case NO_COM:{
			break;
		}
		case START_COM:{
			state = RUNNING_ST;
			break;
		}
		default:{
			//WRONG COMMAND
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void get_distance(){
	long duration, cm;
    Serial.begin (9600);
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10); // В этот момент датчик будет посылать сигналы с частотой 40 КГц.
    digitalWrite(PIN_TRIG, LOW);
    duration = pulseIn(PIN_ECHO, HIGH);//  Время задержки акустического сигнала на эхолокаторе.
    cm = duration / 58 ; //  преобразовать время в расстояние
    Serial.print(cm);
    delay(300);
}
bool check_object(){
	float dist = get_distance();
	if (dist <= OBJ_FIND_DISTANCE_CM){
	return true;
	} else { 
	return false;
	}
}
////////////////////////////////////////////////////////////////////////////////////
void runningSt_handler(int cmd){
	if (delayedStop){
		conveyor_stop();
		state = WAITING_ST; 
		return;
	}
		
	if (check_object()){ // если обьект в зоне обнаружения
		conveyor_stop();// то остановка и переходь в S3
		state = SCANNING_ST;
		return;
	}
	conveyor_run(); // работает постоянно пока первые if не сработают 
	
	switch(cmd){
		case NO_COM:{
			break;
		}
		case STOP_COM:{
			conveyor_stop();
			state = WAITING_ST;
			break;
		}
		default:{
			//WRONG COMMAND
			break;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void scanningSt_handler(int cmd){
	if (delayedStop){
		conveyor_stop();
		state = WAITING_ST;
		return;
	}
	switch(cmd){
		case COLOR_1:{
			state = SCAN_SUCCESS_ST;
			break;
		}
		default:{
			state = SCAN_ERROR_ST;
			break;
		}
	}
}
//////////////////////////////////////////////////////////////////////////////////////
void scanSuccessSt_handler(int cmd){
	switch(cmd){
		case COLOR_1:{
			conveyor_run_2_L(); 
			state = RUNNING_ST;
			break;
		}
		default:{ // прописать выход в S3
			conveyor_run_2_R();
			state = RUNNING_ST;
			break;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////
void scanErrorSt_handler(int cmd){
	Serial.begin(9600);
	Serial.println("Ошибка сканирования, поправте кубики");
	state = SCANNING_ST;
	return;
}
// raspberry comands
#define LEFT 0
#define RIGHT 1
#define NO_COM '0'
#define START_COM '1'
#define STOP_COM '2'
#define SCANNING_ERROR_COM '3'
#define MOVE_R 	'4'
#define MOVE_L	'5'
#define COLOR_1 '1'

int check_data(){
	return Serial.available(); // Получение инфо с малины запись в фунуцию 
}

int get_command(){ 
	if (check_data > 0){
		int cmd = Serial.read();// локальная смд 
	}
	if (cmd < NO_COM) or  (cmd > MOVE_L) //если команда не пуста или команда не подходит под 1 цвет, то ожидание
		cmd = NO_COM;
	return cmd;
}///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(){
	int cmd = get_command();
	if (cmd == STOP_COM){
		delayedStop = true;
	}

	switch(state){
		case WAITING_ST:{ //S1
			waitingSt_handler(cmd);
			break;
		}
		case RUNNING_ST:{ // S2
			runningSt_handler(cmd);
			break;
		}
		case SCANNING_ST:{ //S3
			scanningSt_handler(cmd);
			break;
		}
		case SCAN_SUCCESS_ST:{ //S3 succeses
			scanSuccessSt_handler(cmd);
			break;
		}
		case SCAN_ERROR_ST:{// S3 error
			scanErrorSt_handler(cmd);
			break;
		}
	}
}