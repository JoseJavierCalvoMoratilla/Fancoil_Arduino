#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include "font.h"
#include <OneWire.h>      
#include <ArduinoJson.h>  

// CONFIGURACION WIFI ///////////////////////////////////////////////////////////////////////////////////////////
#define SSID "" // insert your SSID
#define PASS ""// insert your password
WiFiServer server(80); //Puerto que queremos usar para la comunicacion
//Si quieres IP Fija los siguientes datos hay que descomentarlos...
//IPAddress ip(192,168,1,2);  
//IPAddress gateway(192,168,1,1);
//IPAddress subnet(255,255,255,0);
String string_ip = ""; //Se usa para la petición JSON, IP Dispositivo

//Info HTTP Client, 
int    HTTP_PORT   = 8080;
String HTTP_METHOD = "POST"; // or "POST"
char   HOST_NAME[] = "192.168.1.3"; // hostname of web server:
String PATH_NAME   = "/name";
WiFiClient wificlient;
HTTPClient http;

// CONFIGURACION  sensor temperatura ds18b20 /////////////////////////////////////////////////////////////////////
#define ONE_WIRE_BUS 2 // GPIO 12 del ESP8266
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// CONFIGURACION salidas/ entradas digitales /////////////////////////////////////////////////////////////////////
int rele_valvula=12;
int rele_pot1=13;
int rele_pot2=14;
int rele_pot3=16;
int sensor_ir1 = 5;

// Variables Estados /////////////////////////////////////////////////////////////////////////////////////////////
float temperatura = 0.0;
boolean estado_marcha=false; // modificado false
boolean estado_modo=true; // modificado false
//boolean estado_potencia=false;
boolean estado_orden = false;
boolean estado_ir1 = false;

float consigna=26.00; // modificado 20.0
float histeresis = 0.5;
float temp_max = 30.0;
float temp_min = 15.0;
int pot_max = 3;
int pot_min = 1;
int potencia;

int peticiones = 0;

// Prototipos de funciones /////////////////////////////////////////////////////////////////////////////////////////
bool orden(float, float, float, bool);
void comprobar_temperatura(void);
void configuracion(void);
void configuracionGPIO(void);
void lanzarJSON(void);

// INICIO SETUP /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//##########################################################################################################################
void setup(void) {
   
  configuracionGPIO();
  configuracion();

} // FIN SETUP
//##########################################################################################################################


// INICIO LOOP //////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//##########################################################################################################################
void loop(void) {
  comprobar_temperatura();
  delay(100);  

  // Reconexión en caso de pérdida de coenxión /////////////////////////////////////////////////////
  if (WiFi.status() != WL_CONNECTED) {      
    Serial.println("Se ha perdido la conexión con el AP");
    Serial.println("Se ejecuta de nuevo el setup ..." );
    delay(3000);    
    configuracion();    
  }

  //LOGICA DEL FANCOIL /////////////////////////////////////////////////////////////////////////////
   
  if(estado_marcha == true) {
   
  estado_orden = orden(temperatura, consigna, histeresis, estado_modo);
  Serial.println("-------------------");
  Serial.println("Temperatura = " + String(temperatura));
  Serial.println("Consigna = " + String(consigna));
  Serial.println("-------------------");
 
   // Aquí va la función
    if(estado_orden == true) {
      digitalWrite(rele_valvula, HIGH);
      potencia == 1;
     
      }
    else {
      digitalWrite(rele_valvula, LOW);
      potencia == 1;
      }  
 
     
        if(potencia == 1) {
        digitalWrite(rele_pot1,HIGH);
        digitalWrite(rele_pot2,LOW);
        digitalWrite(rele_pot3,LOW);
       
        }
      else if(potencia == 2) {
        digitalWrite(rele_pot1,LOW);
        digitalWrite(rele_pot2,HIGH);
        digitalWrite(rele_pot3,LOW);        
        }    
       else {
        digitalWrite(rele_pot1,LOW);
        digitalWrite(rele_pot2,LOW);
        digitalWrite(rele_pot3,HIGH);
        }  

  }
  else {  
    digitalWrite(rele_valvula, LOW);
    potencia = 1;
    }

  /////////////////////////////////////////////////////////////////////////////////////////////////
 
  // AVISOS a Servidor http////////////////////////////////////////////////////////////////////////////
  if(digitalRead(sensor_ir1)) {
    lanzarJSON();
    delay(1000);
   
    }

// Conectividad WIFI //////////////////////////////////////////////////////////////////////////////////////////
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  while(!client.available()){
    delay(1);
  }
 
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();


 // COMANDOS ////////////////////////////////////////////////////////////////////////////////////////////////////
 
  // ACCION Marcha/Paro /////////////////////////////////////////////////////////////////////////////////////////
  if (req.indexOf("/ONOFF") != -1){  
    peticiones++;
    Serial.println(peticiones);
  //if (peticiones == 3) {
  if (estado_marcha==false){
    estado_marcha=true;
    potencia=1;
    peticiones = 0;  
  }
  else  {
    estado_marcha=false;
    digitalWrite(rele_valvula,LOW);
    digitalWrite(rele_pot1,LOW);
    digitalWrite(rele_pot2,LOW);
    digitalWrite(rele_pot3,LOW);
    peticiones = 0;  
  }
  //}
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
  }

// ACCION estado Marcha/Paro ///////////////////////////////////////////////////////////////////////////////////
 else if (req.indexOf("/ESTADO") != -1){
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    if (estado_marcha==true){
    s += "ON";
    }
    else{
    s += "OFF";
    }
    s += "\n" ;
    client.print (s);
    peticiones = 0;
  }

// ACCION Subir Temperatura ////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/SUBIRT") != -1){
    peticiones++;
  //if (peticiones == 3) {      
      if (consigna<=temp_max){
          consigna=consigna+0.5;          
          Serial.println(consigna);          
      }
     peticiones = 0;    
  //}
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
  }
   

// ACCION bajar Temperatura ////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/BAJART") != -1){

    peticiones++;
  //if (peticiones == 3) {
   
     if (consigna>=temp_min){
       consigna=consigna-0.5;        
        Serial.println(consigna);
    }
    peticiones = 0;
    String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
  //}
 
  }

// ACCION obtener temperatura consigna ////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/CONSIGNA") != -1){
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";  
  s += (consigna);  
  s += "</html>\n";
  client.print (s);  
  peticiones = 0;
}

// ACCION Obtener temperatura real ////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/TEMPERATURA") != -1){
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
  s += (temperatura);
  s += "</html>\n";
  client.print (s);
  peticiones = 0;
}

// ACCION Obtener modo ///////////////////////////////////////////////////////////////////////////////////////
 else if (req.indexOf("/MODO") != -1){
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    if (estado_modo==false){
    s += "FRIO";
    }
    else{
    s += "CALOR";
    }
    s += "\n" ;
    client.print (s);
    peticiones = 0;
  }

// ACCION consultar POTENCIA ///////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/POTENCIA") != -1){
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";  
  s += (potencia);  
  s += "</html>\n";
  client.print (s);  
}
// ACCION SUBIR POTENCIA //////////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/SUBIRP") != -1){

    peticiones++;
  //if (peticiones == 3) {
   
    if (potencia + 1 <= pot_max){      
          potencia=potencia+1;          
          Serial.println(potencia);
         
      }  
      else {
        Serial.println("El límite mínimo de potencia es: " + pot_max);
        }
        peticiones = 0;
  //}
  String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
  }

// ACCION BAJAR POTENCIA //////////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/BAJARP") != -1){
    peticiones++;
  //if (peticiones == 3) {
    if (potencia - 1 >= pot_min){      
          potencia=potencia-1;          
          Serial.println(potencia);
      }
    else {
          Serial.println("El límite mínimo de potencia es: " + pot_min);
      }
      peticiones = 0;
   
    String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
   //}
  }

// ACCION CALOR //////////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/CALOR") != -1)
      {    
          estado_modo = true;      
          Serial.println("Se ha configurado el modo calor");
          String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
          peticiones = 0;
      }

 // ACCION FRIO //////////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/FRIO") != -1)
      {    
          estado_modo = false;      
          Serial.println("Se ha configurado el modo frio");
          String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
          peticiones = 0;
      }

     // ACCION FRIO //////////////////////////////////////////////////////////////////////////////////////////
  else if (req.indexOf("/TESTJSON") != -1)
      {    

          lanzarJSON();
          String s ="HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n";
s += ("OK");
s += "</html>\n";
client.print (s);
          peticiones = 0;
      }

} // FIN LOOP
//##########################################################################################################################

// Funciones ///////////////////////////////////////////////////////////////////////////////////////
void comprobar_temperatura()
{
  sensors.requestTemperatures();
  temperatura=sensors.getTempCByIndex(0);
  //temperatura = temperatura - 2.00; // resta o suma aqui la diferencia de temperatura con otro tipo de termometro o comenta la linea si no hay que compensar nada...
}

bool orden(float temperatura, float consigna, float histeresis, bool estado_modo) {
 
      if((temperatura > (consigna + histeresis)) && estado_modo == true) { // Modo calor
      Serial.println("Se ha cumplido la temperatura consigna objetivo con el modo calor.");
      return false;      
      }
         
    else if((temperatura < (consigna - histeresis)) && estado_modo == false) {
      Serial.println("Se ha cumplido la temperatura consigna objetivo con el modo frio.");
      return false;        
      }  

     else if((temperatura < (consigna - histeresis)) && estado_modo == true) {  //MODO CALOR
      Serial.println("Se pone en marcha el FANCOIL modo calor.");
      return true;      
    }
   
    else if((temperatura > (consigna + histeresis)) && estado_modo == false){ //MODO FRIO      
      Serial.println("Se pone en marcha el FANCOIL modo frio.");
      return true;
    }  
  }

void configuracion(void) {  
 
  Serial.begin(115200);                          
  Wire.begin(0,2);                        
  WiFi.begin(SSID, PASS);
 
 
  //Si queremos la IP fija, descomentaremos la siguiente linea
  //################################################################  
  //WiFi.config(ip, gateway, subnet);
  //################################################################
 
  // Se manda por puerto serie el estado de la conectividad del chip WIFI
  while (WiFi.status() != WL_CONNECTED) {         // Espera a la conexi´n de la red WiFi
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nLa conexión del dispositivo con la red wifi ha sido satisfactoria");
  Serial.println(WiFi.localIP());


  char result[16];
  sprintf(result, "%3d.%3d.%3d.%3d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
 
  server.begin();              
  delay(5000);
  //pinMode(rele_valvula,OUTPUT);
  //setMarcha(0);  
  //estado_marcha=false;  
  sensors.begin();
 
  }

  void configuracionGPIO(void) {

      // Se configuran las salidas de los reles
      pinMode(rele_valvula,OUTPUT);
      pinMode(rele_pot1,OUTPUT);
      pinMode(rele_pot2,OUTPUT);
      pinMode(rele_pot3,OUTPUT);
      pinMode(sensor_ir1, INPUT);

      digitalWrite(rele_valvula,LOW);
      digitalWrite(rele_pot1,LOW);
      digitalWrite(rele_pot2,LOW);
      digitalWrite(rele_pot3,LOW);
 
   
    }

void lanzarJSON(void){  

  if(wificlient.connect(HOST_NAME, HTTP_PORT)) {    
    Serial.println("Connected to http server");  
    http.begin(wificlient, "http://" + String(HOST_NAME) + ":" + String(HTTP_PORT) + String(PATH_NAME));

    String json = "{\"ip\":\""+ string_ip + "\",\"modules\":{ \"presencia\": 1}}";
   
    // Specify content-type header
      http.addHeader("Content-Type", "application/json");
      //http.addHeader("Content-Type", "text/plain");
      // Data to send with HTTP POST              
      // Send HTTP POST request
      int httpResponseCode = http.POST(json);

      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
       
      // Free resources
      http.end();

  }  
 
  }
