//LIBRERÍAS****************************************************************************************************************************************************************************************************

//Librerias para uso de memora micro SD
//Módulo SIM GSM
#define ledDeConexionaRed 8 // Pin digital 8 para indicar si hay conexión a red    
#define resetSim800l 5 //Pin digital 5 asignado para resetear el modulo SIM800L
int contadorDeIntentosDeConexion;

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

  //Intentar conexión a red
  contadorDeIntentosDeConexion=0;
  Serial.println("INICIA INTENTO DE CONEXIÓN A RED");
  Serial.println();
  //sendAtConectionRed();
  delay(500);
  Serial.println("SE LOGRÓ LA CONEXIÓN A RED");
  Serial.println();
  Serial.println();

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