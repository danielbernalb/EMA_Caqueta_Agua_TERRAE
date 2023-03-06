//LIBRERÍAS****************************************************************************************************************************************************************************************************

//Librerias para uso de memora micro SD
#include <SD.h>
#include <SPI.h>

//Librerías para RTC Reloj de Tiempo Real
#include <RTClib.h>

//Librería para uso de memoria interna del Arduino EEPROM
#include <EEPROM.h>

//Librerías para separar información por el caracter ";"
#include <Separador.h>

//Librería para sensor conductividad
#include "GravityTDS.h"




//VARIABLES Y CONSTANTES DEL PROGRAMA ***************************************************************************************************************************************************************************

//Módulo SIM GSM
#define ledDeConexionaRed 8 // Pin digital 8 para indicar si hay conexión a red    
#define resetSim800l 5 //Pin digital 5 asignado para resetear el modulo SIM800L
int contadorDeIntentosDeConexion;

//Módulo micro SD
#define pinLectorSD 53 //Pin para lectura de información del modulo microSD  
File file;             //Objeto file que nos servira para manipular archivos
Separador s, z, y;     //Se crea objeto para separar mensaje.

//Módulo RTC
RTC_DS3231 rtc;        //Se crea objeto del tipo RTC_DS3231 como Reloj de Tiempo Real

//Módulo para sensor distancia ultrasonido
const int echoPin = 6;
const int triggerPin = 7;

//Módulo conductividad eléctrica
GravityTDS gravityTds;
#define TdsSensorPin A1
#define VREF 5.0      // analog reference voltage(Volt) of the ADC
#define SCOUNT  30           // sum of sample point
float averageVoltage = 0, tdsValue = 0, temperature = 25;
float factorCE = 1;


//Variables lectura informacion SD
String primerComilla;
String cuerpoMensaje;
bool leerSD;

//VAriables envio de mensajes SMS
String commands;
String numMovil;
String numString;
String bodyMsm;
String info;
String dataInfo;
bool a = false;
bool enviarMensaje = true;
int8_t answerAtConectionRed;
int errorAtConectionRed = 0;
int conteoMedida = 0;
int conteoInterno = 0;
unsigned long timeSendMSN = 0;     //Contador para envio mensaje
unsigned long tiempoPrevioMSN;     //Contador para control envio mensaje


//Variables personalizables
String datosInforme[] = {"", "", "", "", "", "", "", ""}; //Datos que contiene el informe que se envia a diario
String datos[] = {"", "", "", "", "", "", "", ""}; // Array que contiene los datos que personalizan el programa y numeros para enviar mensajes
#define GetSize(datos) (sizeof(datos)/sizeof(*(datos)))
int canDatos = GetSize(datos);
int maximaCE;
int medida;
int alturaSensor;
int medidaCE;
int horaParaEnvioDeInforme; //Hora para envio de informe

//****************************************************************************************************************************************************************************************************************************

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);

  
  //Configuración de pines
  pinMode(resetSim800l, OUTPUT);
  pinMode(ledDeConexionaRed,    OUTPUT);
  pinMode(pinLectorSD,   OUTPUT);
  digitalWrite(resetSim800l, HIGH);
  pinMode(TdsSensorPin, INPUT);
  pinMode(triggerPin, OUTPUT); //pin como salida
  pinMode(echoPin, INPUT);  //pin como entrada
  digitalWrite(triggerPin, LOW);//Inicializamos el pin con 0
  gravityTds.setPin(TdsSensorPin);
  gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
  gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
  gravityTds.begin();  //initialization


  //Intentar conexión a red
  contadorDeIntentosDeConexion=0;
  Serial.println("INICIA INTENTO DE CONEXIÓN A RED");
  Serial.println();
  //sendAtConectionRed();
  delay(500);
  Serial.println("SE LOGRÓ LA CONEXIÓN A RED");
  Serial.println();
  Serial.println();

  //Mostrar datos de monitoreo guardados en memoria del Arduino
  Serial.println("A continuación se muestran los valores guardados en la memoria para el próximo reporte diario");
  Serial.println(readEEPROM(22, 100));
  Serial.println(readEEPROM(22, 200));
  Serial.println(readEEPROM(22, 300));
  Serial.println(readEEPROM(22, 400));
  Serial.println(readEEPROM(22, 500));
  Serial.println(readEEPROM(22, 600));
  Serial.println(readEEPROM(22, 700));
  Serial.println(readEEPROM(22, 800));
  Serial.println();
  Serial.println();


  //Inicio módulo RTC
  if (! rtc.begin()) {                            // si falla la inicializacion del modulo
    Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
    Serial.flush();
    abort();
  }
  delay(500);


  //Iniciar módulo micro SD
  if (!SD.begin(pinLectorSD)) {
    Serial.println("Modulo micro SD no encontrado");
    while (1);
  }
  delay(500);
  leerSD = true;
  Serial.println("INICIA IMORTACIÓN DE DATOS DESDE MEMORIA MICRO SD, LOS DATOS SON LOS SIGUIENTES:");
  separateData("");


  //Configuración parámetros-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
  timeSendMSN = 1800000;               // Tiempo para envio de mensajes: 1000 es un segundo
  alturaSensor = (datos[1]).toInt();
  maximaCE = (datos[2]).toInt();
  horaParaEnvioDeInforme = 17;

  Serial.println();
  Serial.println();
  Serial.print("El actual factor de calibración para el sensor de conductividad es: ");
  EEPROM.get(1000, factorCE); //El valor por defecto fue 0356, para modificarlo correr este código una vez: EEPROM.write(800, factorCE);
  Serial.println(factorCE);  
  medida = medirdistancia(triggerPin, echoPin);
  medidaCE = medirCE();
  Serial.println();
  Serial.println();  
  Serial.print("Primer medida realizada con exito el ");
  DateTime fecha = rtc.now();
  Serial.print(fecha.day());
  Serial.print("/");
  Serial.print(fecha.month());
  Serial.print(", a las ");
  Serial.print(fecha.hour());
  Serial.print(":");
  Serial.println(fecha.minute());
  Serial.print("CE: ");
  Serial.print(medidaCE);
  Serial.print("µS, Nivel: ");
  Serial.print(medida);
  Serial.println("cm");
  Serial.println();


  conteoMedida = EEPROM.read(15); // Actualiza el contador desde la memoria de arduino
  conteoInterno = EEPROM.read(20); // Actualiza el contador desde la memoria de arduino
  enviarMensaje = EEPROM.read(1100); // Actualiza el contador desde la memoria de arduino
  //enviarMensaje=true;

  Serial.print("conteoMedida(0-5): ");
  Serial.println(conteoMedida);
  Serial.print("conteoInterno(0-7): ");
  Serial.println(conteoInterno);
  Serial.print("Se enviará mensaje?: ");
  Serial.println(enviarMensaje);
  Serial.println();
  Serial.println();

  Serial.println("Sistema de seguimiento iniciado correctamente...");
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println();
}


//************************************************************************************************************************************************************************************************************************

void loop() {
  Serial.println("*");

  ComprobarInformacionDeEntrada();


  DateTime fecha = rtc.now();
  if ( fecha.hour() == horaParaEnvioDeInforme  and enviarMensaje == true )
  { envioInforme();
    enviarMensaje = false;
    EEPROM.write(1100, enviarMensaje);
  }

  if (fecha.hour() != horaParaEnvioDeInforme) {
    enviarMensaje = true;
    EEPROM.write(1100, enviarMensaje);
  }

  readLecturaMedidas();// Intenta tomar medidas, solo lo hará cada 30 minutos(o el tiempo indicado)
}





//FUNCIONES Y MÓDULOS***************************************************************************************************************************************************************************************************




void ComprobarInformacionDeEntrada() {
  Serial.println("Buscando si hay mensaje");
  bodyMsm = readSIM800L(); //Leer lo que viene en mensajes de texto
  separateData(bodyMsm);// Separar texto del mensaje leido para decodificar la palabra clave "nivel", se cambia el valor de a
  if (a) {
    if ( commands.equalsIgnoreCase("nivel") ) {
      Serial.println("llegó mensaje nivel");
      a = false;
      //sendAtConectionRed();  //Configuracion para enviar mensaje.

      Serial1.println("AT+CMGS=\"" + numMovil + "\""); // Numero para enviar mensaje
      delay(500);
      Serial1.print("El nivel en este momento es: ");
      Serial1.print(medirdistancia(triggerPin, echoPin));
      Serial1.print(", la conductividad en este momento es: ")
      ;
      Serial1.print(medirCE());
      Serial1.print(" uS, en la fecha: ");
      DateTime fecha = rtc.now();
      info = String(fecha.day(), DEC) + "/" + String(fecha.month(), DEC) + "," + String(fecha.hour(), DEC) + ":" + String(fecha.minute(), DEC);
      Serial1.print(info);
      delay(1000);
      Serial1.write(0x1A); //ctrl+z
      delay(4000);

      commands = "";
    }

    if ( commands.substring(0, 3).equalsIgnoreCase("ce:") ) {
      a = false;
      factorCE = (commands.substring(3, commands.length())).toFloat();
      EEPROM.put(1000, factorCE);
      Serial.println("Factor CE actualizado");
      commands = "";
    }
  }

}


//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCION PARA TOMAR MEDICIONES CADA TIEMPO DETERMINADO
void readLecturaMedidas() {

  unsigned long tiempoActualMSN = millis(); //Inicio contador
  if ((unsigned long)(tiempoActualMSN - tiempoPrevioMSN) >= timeSendMSN) {


    //sendAtConectionRed();
    medida = medirdistancia(triggerPin, echoPin);
    delay(1000);
    medidaCE = medirCE();
    delay(1000);
    EEPROM.write(10, medida);
    delay(1000);
    EEPROM.write(40, medidaCE);
    Serial.print("Conteo repetición: ");
    Serial.println( conteoMedida + 1);
    Serial.print("Conteo Serie: ");
    Serial.println( conteoInterno);    
    Serial.print("Medida nivel: ");
    Serial.print(medida);
    //Serial.print(EEPROM.read(10));
    Serial.println(" cm");
    Serial.print("Medida CE: ");    
    Serial.print(medidaCE);
    Serial.println("µS");
    Serial.println();
    //verDatosInforme();
    Serial.println("");
    // Serial.print(EEPROM.read(40));
    conteoMedida++;
    EEPROM.write(15, conteoMedida);
    if (conteoMedida > 5) {
      conteoMedida = 0;
      EEPROM.write(15, conteoMedida);


      //        medida  = EEPROM.read(10);
      //        medidaCE = EEPROM.read(40);
      DateTime fecha = rtc.now();
      info = String(fecha.day(), DEC) + "/" + String(fecha.month(), DEC) + "," + String(fecha.hour(), DEC) + ":" + String(fecha.minute(), DEC) + ";" + medida + ";" + medidaCE;
      Serial.println(info);
      datosInforme[conteoInterno] = info;
      dataInfo = datosInforme[conteoInterno];

      EEPROM.write(20, conteoInterno);
      saveDataInfo(conteoInterno, dataInfo);
      conteoInterno++;
      if (conteoInterno > 7) {
        conteoInterno = 0;
        EEPROM.write(20, conteoInterno);
      }

      delay(500);
      file = SD.open("datos.csv", FILE_WRITE);
      if (file) {
        file.print(fecha.day());
        file.print("/");
        file.print(fecha.month());
        file.print(",");
        file.print(fecha.hour());
        file.print(":");
        file.print(fecha.minute());
        file.print(";");
        file.print(medida);
        file.print(";");
        file.println(medidaCE);
        file.close();
        Serial.print(conteoInterno);
        Serial.println("° Registro exitoso para informe diario");
        Serial.println("");

      } else {
        Serial.println("Falla en registro");
      }
    }

    if (medidaCE > maximaCE)
      envioMensajeAlerta();

    tiempoPrevioMSN = tiempoActualMSN;
  }
}






//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA ENVIAR INFORME DIARIO
void envioInforme() {

  datosInforme[0] = readEEPROM(22, 100);
  datosInforme[1] = readEEPROM(22, 200);
  datosInforme[2] = readEEPROM(22, 300);
  datosInforme[3] = readEEPROM(22, 400);
  datosInforme[4] = readEEPROM(22, 500);
  datosInforme[6] = readEEPROM(22, 600);
  datosInforme[6] = readEEPROM(22, 700);
  datosInforme[7] = readEEPROM(22, 800);
  delay (2000);
  //sendAtConectionRed();
  delay(1000);

  for (int i = 3; i <= 6; i++) {
    if ((datos[i]).length() > 0) {
      Serial1.println("AT+CMGS=\"" + datos[i] + "\""); // Numero para enviar mensaje
      Serial.println("AT+CMGS=\"" + datos[i] + "\""); // Numero para enviar mensaje
      delay(500);
      for (int j = 0; j < 8; j++) {
        String dataInfo = datosInforme[j];
        Serial1.print(dataInfo);  // Mensaje a enviar
        Serial1.print(", ");
      }

      delay(1000);
      Serial1.write((char)26); //ctrl+z
      delay(9000);
    }
  }
  answerAtConectionRed = 0;



  writeEEPROM("**********************", 100);
  writeEEPROM("**********************", 200);
  writeEEPROM("**********************", 300);
  writeEEPROM("**********************", 400);
  writeEEPROM("**********************", 500);
  writeEEPROM("**********************", 600);
  writeEEPROM("**********************", 700);
  writeEEPROM("**********************", 800);



  
}







//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA ENVÍO DE MENSAJES DE ALERTA
void envioMensajeAlerta() {

//Serial.println("Se envía mensaje de alerta");
//    delay (2000);
//    //sendAtConectionRed();
//    delay(1000);
//
//  for (int i = 3; i <= 6; i++) {
//    if ((datos[i]).length() > 0) {
//      Serial1.println("AT+CMGS=\"" + datos[i] + "\""); // Numero para enviar mensaje
//      delay(500);
//      Serial1.print("Alerta, el nivel del agua es de: ");  // Mensaje a enviar
//      Serial1.print(medida);  // Mensaje a enviar
//      Serial1.print(", y el nivel de conductividad es de: ");  // Mensaje a enviar
//      Serial1.print(medidaCE);  // Mensaje a enviar
//      delay(1000);
//      Serial1.write((char)26); //ctrl+z
//      delay(9000);
//    }
//  }
//  answerAtConectionRed = 0;
}





//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA CONFIGURAR MÓDULO SIM800L

void sendAtConectionRed() {

  
  while ( (answerAtConectionRed == 0) && (contadorDeIntentosDeConexion<2) ) {
    digitalWrite(ledDeConexionaRed, LOW);
    answerAtConectionRed = enviarComandoAT("AT+CREG?", "+CREG: 0,1", 1000) || enviarComandoAT("AT+CREG?", "+CREG: 0,5", 1000);
    Serial.println("No hay respuesta al intento de conexión");
    errorAtConectionRed++;
    if (errorAtConectionRed > 8) {
      errorAtConectionRed = 0;
      delay(2000);
      Serial.println("Reiniciando Sim800l...");
      digitalWrite(resetSim800l, LOW);
      delay(500);
      digitalWrite(resetSim800l, HIGH);
      delay(15000);
      contadorDeIntentosDeConexion++;
    }


    if (answerAtConectionRed == 1) {
      Serial.println("Si hay respuesta al intento de conexión");
      errorAtConectionRed = 0;
      enviarComandoAT("AT", "OK", 1000);
      enviarComandoAT("AT+CLIP=1", "OK", 1000);
      enviarComandoAT("AT+CNMI=2,2,0,0,0", "OK", 1000); /* Transmisión de mensaje directamente al puerto serial del dispositivo. */
      enviarComandoAT("AT+CMGD=1,4", "OK", 1000);
      enviarComandoAT("AT+CMGL=\"ALL\",0", "OK", 2000);
      enviarComandoAT("AT+CMGF=1", "OK", 2000);
      digitalWrite(ledDeConexionaRed, HIGH);
    }
  }

  contadorDeIntentosDeConexion=0;
}



//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA MEDIR CE
int medirCE() {
  //temperature = readTemperature();  //add your temperature sensor and read it
  gravityTds.setTemperature(temperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
  //    Serial.print(tdsValue,0);
  //    Serial.println("ppm");
  //    delay(1000);
  //Serial.println(factorCE);
  return (tdsValue * factorCE);
}



//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA SACAR EL VALOR PROMEDIO DE LA CE
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}






//-----------------------------------------------------------------------------------------------------------------------------------------------//
//función para escribir en la memoria interna de arduino eeprom
void writeEEPROM(String dto, int posIni) {
  for (int i = posIni; i < (dto.length() + posIni); i++) {
    EEPROM.write(i, dto[i - posIni]);
  }
  delay(400);
}






//-----------------------------------------------------------------------------------------------------------------------------------------------//
//función para leer de la memoria interna de arduino eeprom
String readEEPROM(int t, int posIni) {

  String texto;
  Serial.println();
  for (int i = posIni; i < (t + posIni); i++) {
    byte z = EEPROM.read(i);

    char a = char(z);
    texto += a;
  }
  delay(400);
  return texto;
}






//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA GUARDAR DATOS DEL INFORME EN POSICIONES ESPECÍFICAS DE LA EEPROM
void saveDataInfo(int conteoInterno, String dataInfo) {
  Serial.println(dataInfo);
  
  switch (conteoInterno) {
    case 0: writeEEPROM(dataInfo, 100); break;
    case 1: writeEEPROM(dataInfo, 200); break;
    case 2: writeEEPROM(dataInfo, 300); break;
    case 3: writeEEPROM(dataInfo, 400); break;
    case 4: writeEEPROM(dataInfo, 500); break;
    case 5: writeEEPROM(dataInfo, 600); break;
    case 6: writeEEPROM(dataInfo, 700); break;
    case 7: writeEEPROM(dataInfo, 800); break;
  }
}





//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCION PARA ENVIAR COMANDOS AT SEND COMMANDS AT
int8_t enviarComandoAT(char* comandoAT, char* respEsperada, unsigned int retardo) {

  uint8_t x = 0,  responde = 0;
  char respuesta[100];
  unsigned long anterior;

  memset(respuesta, '\0', 100);    // Borra el contenido del array
  delay(100);

  while ( Serial1.available() > 0)
    Serial1.read();                  // Limpie el buffer de entrada

  if (comandoAT[0] != '\0') {
    Serial1.println(comandoAT);    // Enviar comando AT
    //      Serial.println(comandoAT);  // Mostrar comando por serial
  }

  x = 0;
  anterior = millis();

  // Este bucle espera la respuesta
  do {
    if (Serial1.available() != 0) {  // Si hay datos en el búfer de entrada de UART, léalo y verifica la respuesta
      respuesta[x] = Serial1.read();
      //Serial.print(response[x]);
      x++;
      if (strstr(respuesta, respEsperada) != NULL) {   // Verificar si la respuesta deseada (OK) está en la respuesta del módulo
        responde = 1;
      }
    }
  } while ((responde == 0) && ((millis() - anterior) < retardo));   // Espera la respuesta con tiempo de espera

  return responde;
}




//-----------------------------------------------------------------------------------------------------------------------------------------------//
//LEER DATOS DESDE LA MEMORIA SD CREANDO UN STRING LLAMADO TEXTO
String readDataSD() {

  String texto;
  bool val = SD.begin(pinLectorSD);
  if (not val)
    Serial.println("No se ha podido inicializar la tarjeta SD");
  else {
    file = SD.open("fichero.txt", FILE_READ); //nOMBRE DEL FICHERO QUE SE LEE
    if (not file)
      Serial.println("No se ha podido abrir el fichero");
    else {

      char c = file.read();
      while (c != -1) { //Leemos hasta el final
        //Serial.print(c);
        c = file.read();
        texto += c;
      }
      file.close();
    }
  }
  return texto;
}



//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCION para leer información de los mensajes de texto para mandar informe si es solicitado, retorna el texto del mensaje

String readSIM800L() {
  String texto;

  unsigned long millisAntes = millis();
  unsigned long millisDespues = millisAntes;

  while ( millisDespues - millisAntes < 500) { // Estaba en 4000
    while (Serial1.available() > 0) {
      char aux = (char)Serial1.read();
      texto += aux;
    }
    millisDespues = millis();
  }
  return texto;
}



//-----------------------------------------------------------------------------------------------------------------------------------------------//
//Separa textos por los punto y coma que contienen. Esta función tambien lee la información de la memoria SD
void separateData(String dato) {

  if (dato.startsWith("\r\n+CMT: ")) {
    primerComilla = s.separa(dato, '"', 0);
    numMovil = s.separa(dato, '"', 1);
    cuerpoMensaje = s.separa(dato, '\n', 2);
    numString = z.separa(cuerpoMensaje, ';', 0);
    commands = z.separa(cuerpoMensaje, ';', 1);
    //        numInt = numString.toInt();

    Serial.print("Numero:");
    Serial.println(numMovil);
    //        Serial.print("Caso:");
    //        Serial.println(numInt);
    Serial.print("Parametro:");
    Serial.println(commands);
    a = true;
  }

  if (leerSD) {
    for (int i = 0; i < canDatos; i++) {
      datos[i] = s.separa(readDataSD(), ';', i);
    }

    delay(10);
    for (int j = 1; j < canDatos; j++) {
      Serial.print("Dato No ");
      Serial.print(j);
      Serial.println(" : " + datos[j]);
    }
    leerSD = false;
  }
}

void EnviaSMS(String texto, String numero) {
  Serial1.println("AT+CMGF=1");                 // Activamos la funcion de envio de SMS
  delay(100);                                    // Pequeña pausa
  Serial1.println("AT+CMGS=\"+57" + numero + "\""); // Definimos el numero del destinatario en formato internacional 3014124338
  delay(100);                                    // Pequeña pausa
  Serial1.print(texto);                         // Definimos el cuerpo del mensaje
  delay(500);                                    // Pequeña pausa
  Serial1.print(char(26));                      // Enviamos el equivalente a Control+Z
  delay(100);                                    // Pequeña pausa
  Serial1.println("");                          // Enviamos un fin de linea
  delay(100);                                    // Pequeña pausa
}

//-----------------------------------------------------------------------------------------------------------------------------------------------//
//FUNCIÓN PARA MEDIR ALTURA DEL AGUA, RETORNA EL VALOR PROMEDIO DE 10 MEDICIONES
int medirdistancia(int trigger, int echo) {

  float valorSensorAcumulado = 0;
  long duration, distanceCm;

  for (int i = 0; i < 10; i++) {
    digitalWrite(trigger, LOW);         //para generar un pulso limpio ponemos a LOW 4us
    delayMicroseconds(4);
    digitalWrite(trigger, HIGH);        //generamos Trigger (disparo) de 10us
    delayMicroseconds(10);
    digitalWrite(trigger, LOW);

    duration = pulseIn(echo, HIGH);     //medimos el tiempo entre pulsos, en microsegundos

    distanceCm = duration * 10 / 292 / 2;  //convertimos a distancia, en cm
    delay(100);
    valorSensorAcumulado += distanceCm;
  }
  return alturaSensor - (valorSensorAcumulado / 10);
}
