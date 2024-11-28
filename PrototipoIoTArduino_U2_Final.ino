// Librerías a utilizar
#include <Servo.h>
#include "MatrixKeypad.h"  
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// VARIABLES A UTILIZAR + CONFIGURACIONES ---------------------------------------------------------------------------
// Definimos la longitud de la contraseña
#define Password_Length 5
// Definimos delay al presionar teclas del teclado
#define DEBOUNCE_TIME 1500

// SERVO MOTOR
Servo myservo;
int pos = 0;
bool door = false;

// PANTALLA LCD + I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Dirección del LCD I2C

// TECLADO KEYPAD 4X4
const byte FILAS = 4;
const byte COLUMNAS = 4;
// Pines y mapa de teclas para MatrixKeypad
uint8_t rowPins[FILAS] = {2, 3, 4, 5};
uint8_t colPins[COLUMNAS] = {6, 7, 8, 9};
// Mapeo para keypad normal
char keymap[FILAS][COLUMNAS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
MatrixKeypad_t *customKeypad;  // Instancia del keypad
char customKey; // Tecla presionada

// CONTRASENIA
char Data[Password_Length];
char Master[Password_Length] = "1234";
byte data_count = 0;
bool passwordPromptShown = false;

// LEDs Y BUZZER
int buzzer = A0;
int ledRojo = A1;
int ledVerde = A2;

// LÓGICA DEL SISTEMA  ----------------------------------------------------------------------------
// MÉTODO SETUP
void setup() {
  Serial.begin(9600); // Conexión con el monitor serial
  
  // Inicialización del LCD I2C
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Asignamos cada fila y columna del pad numérico a su correspondiente pin
  for (int i = 0; i < FILAS; i++) {
    pinMode(rowPins[i], INPUT_PULLUP); 
  }
  
  // Creamos la instancia del keypad  // Inicializar MatrixKeypad
  customKeypad = MatrixKeypad_create((char*)keymap, rowPins, colPins, FILAS, COLUMNAS);

  // Asignamos el pin 13 al servomotor
  myservo.attach(13);

  // Asignamos los pines tipo salida a los LEDs y Buzzer
  pinMode(buzzer, OUTPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  // Cerramos la puerta al iniciar el sistema
  ServoClose();
  
  // Indicamos un mensaje de carga al iniciar el sistema
  loading("Cargando");
}

// MÉTODO LOOP
void loop() {
  MatrixKeypad_scan(customKeypad);  // Escanear el keypad
  
  Open(); // Llamamos al método Open() para iniciar el bucle de apertura de puerta
}

/*--- Cerrar la puerta (servo) ---*/
void ServoClose() {
  for (pos = 90; pos >= 30; pos -= 10) {
    myservo.write(pos);
    delay(100);
  }
}

/*--- Abrir la puerta (servo) ---*/
void ServoOpen() {
  for (pos = 0; pos <= 120; pos += 10) {
    myservo.write(pos);
    delay(100);
  }
}

/*--- Simula limpiar el monitor serial y la pantalla LCD ---*/
void clearSerialMonitor() {
  for (int i = 0; i < 20; i++) {  // Imprimir 20 líneas en blanco
    Serial.println();
  }
  lcd.clear();
}

/*--- Repetirá una alarma para el buzzer y LED rojo ---*/
void alarma() {
  for (int i = 0; i < 5; i++) {  // Repetir la alarma 5 veces
    digitalWrite(ledRojo, HIGH);  // Encender el LED rojo
    tone(buzzer, 1000);  // Emitir sonido en el buzzer a 1000 Hz
    delay(500);  // Esperar 500ms
    digitalWrite(ledRojo, LOW);  // Apagar el LED rojo
    noTone(buzzer);  // Apagar el sonido del buzzer
    delay(500);  // Esperar otros 500ms
  }
}

/*--- Muestra un mensaje de carga en el monitor y LCD ---*/
void loading(char msg[]) {
  Serial.print(msg);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  for (int i = 0; i < 9; i++) {
    delay(500);
    Serial.print(".");
    lcd.print(".");
  }
  Serial.println("");
}

/*--- Reinicia los datos del sistema (contraseña ingresada) ---*/
void clearData() {
  while (data_count != 0) {
    Data[--data_count] = 0;
  }
  passwordPromptShown = false;  // Permitir que se muestre el mensaje de nuevo
}

/*--- Función para manejar el proceso de abrir la puerta ---*/
void Open() {
  // Si la contraseña no ha sido ingresada, se muestra el mensaje
  if (!passwordPromptShown) {
    clearSerialMonitor();
    Serial.println("Ingrese Password");
    lcd.setCursor(0, 0);
    lcd.print("Ingrese Password");
    passwordPromptShown = true;
  }

  // Se verifica que se haya presionado una tecla
  if (MatrixKeypad_hasKey(customKeypad)) {
    // Se toma el valor presionado
    customKey = MatrixKeypad_getKey(customKeypad);
    Data[data_count] = customKey; // Se almacena el carácter en una variable
    Serial.print(customKey); 
    lcd.setCursor(data_count, 1);
    lcd.print(customKey); 
    data_count++; // Indicamos que se ha añadido un carácter más

    // Se valida la longitud de la contraseña (mínimo 4 carácteres)
    if (data_count == Password_Length - 1) {
      Data[data_count] = '\0';

      // Se compara la contraseña ingresada con la del sistema
      if (!strcmp(Data, Master)) {
        clearSerialMonitor();
        digitalWrite(ledVerde, HIGH);
        tone(buzzer, 500, 1000);
        // Si la contraseña ingresada es la misma que la del sistema, se abre la puerta
        ServoOpen();
        Serial.println("Puerta Abierta");
        lcd.setCursor(0, 0);
        lcd.print("Puerta Abierta");
        door = true; // Se establece en true la puerta
        // Luego de pasado un tiempo, se vuelve a cerrar la puerta
        delay(5000);  // 5 segundos
        loading("Esperando");  // 4.5 segundos + 5 = Puerta abierta durante 9.5 segundos
        Serial.println("Cerrando puerta");
        lcd.setCursor(0, 0);
        lcd.print("Cerrando puerta");
        digitalWrite(ledVerde, LOW); 
        delay(1000);
        ServoClose(); // Se cierra la puerta
        door = false;
      } else {
        // En caso de que la password no sea la misma, se indica un mensaje con el error
        Serial.println("");
        Serial.println("Pass incorrecta");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Pass incorrecta");
        alarma(); // Suena una alarma para retroalimentar la situación
        delay(1000);
        loading("Cargando");
      }
      // Luego de la operación, se restablece la contraseña ingresada (se limpia)
      clearData();
    }
  }
}


