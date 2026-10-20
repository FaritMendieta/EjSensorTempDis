
/*
  PROYECTO: Sistema de monitoreo con ESP32 y Adafruit IO

  Funciones:
  1. Medir temperatura.
  2. Medir humedad.
  3. Medir distancia.
  4. Generar 4 niveles de alerta segun la temperatura.
  5. Enviar los datos a Adafruit IO.
*/

// ==================================================
// 1. INCLUIR LAS LIBRERIAS
// ==================================================

// Permite conectar el ESP32 a una red WiFi.
#include <WiFi.h>

// Permite enviar datos a Adafruit IO mediante MQTT.
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

// Permite utilizar el sensor de temperatura y humedad.
#include <DHT.h>


// ==================================================
// 2. CONFIGURAR WIFI Y ADAFRUIT IO
// ==================================================

// Nombre y contrasena de tu red WiFi.
#define WIFI_SSID "TU_WIFI"
#define WIFI_PASSWORD "TU_PASSWORD"

// Usuario y clave de tu cuenta Adafruit IO.
#define AIO_USERNAME "TU_USUARIO"
#define AIO_KEY "TU_CLAVE"


// ==================================================
// 3. CONFIGURAR LA CONEXION MQTT
// ==================================================

// Crear el cliente que utiliza la conexion WiFi.
WiFiClient cliente;

// Conectarse al servidor de Adafruit IO.
Adafruit_MQTT_Client mqtt(
  &cliente,
  "io.adafruit.com",
  1883,
  AIO_USERNAME,
  AIO_KEY
);


// ==================================================
// 4. CREAR LOS FEEDS DE ADAFRUIT IO
// ==================================================

// Feed para guardar la temperatura.
Adafruit_MQTT_Publish tempFeed =
  Adafruit_MQTT_Publish(
    &mqtt, AIO_USERNAME "/feeds/temperatura"
  );

// Feed para guardar la humedad.
Adafruit_MQTT_Publish humFeed =
  Adafruit_MQTT_Publish(
    &mqtt, AIO_USERNAME "/feeds/humedad"
  );

// Feed para guardar la distancia.
Adafruit_MQTT_Publish distFeed =
  Adafruit_MQTT_Publish(
    &mqtt, AIO_USERNAME "/feeds/distancia"
  );

// Feed para guardar el nivel de alerta.
Adafruit_MQTT_Publish alertaFeed =
  Adafruit_MQTT_Publish(
    &mqtt, AIO_USERNAME "/feeds/nivel-alerta"
  );


// ==================================================
// 5. CONFIGURAR LOS SENSORES
// ==================================================

// El DHT11 mide temperatura y humedad.
// Su pin de datos esta conectado al GPIO 14.
#define DHTPIN 14
#define DHTTYPE DHT11

// Crear el objeto del sensor DHT11.
DHT dht(DHTPIN, DHTTYPE);

// Pines del sensor ultrasonico HC-SR04.
#define TRIG 18
#define ECHO 19


// ==================================================
// 6. CONFIGURACION INICIAL
// ==================================================

void setup() {

  // Iniciar la comunicacion con el monitor serie.
  Serial.begin(115200);

  // TRIG envia el pulso del sensor ultrasonico.
  pinMode(TRIG, OUTPUT);

  // ECHO recibe el pulso reflejado.
  pinMode(ECHO, INPUT);

  // Iniciar el sensor de temperatura y humedad.
  dht.begin();

  // Conectar el ESP32 a la red WiFi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Esperar hasta que el WiFi este conectado.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Conectando a WiFi...");
  }

  Serial.println("WiFi conectado correctamente.");
}


// ==================================================
// 7. PROGRAMA PRINCIPAL
// ==================================================

void loop() {

  // ----------------------------------------------
  // A. CONECTAR CON ADAFRUIT IO
  // ----------------------------------------------

  // Si MQTT esta desconectado, intentar conectarse.
  if (!mqtt.connected()) {

    int resultado = mqtt.connect();

    // Si ocurre un error, cerrar la conexion
    // y volver a intentar en la siguiente iteracion.
    if (resultado != 0) {
      Serial.println("Error al conectar con Adafruit IO.");
      mqtt.disconnect();
      delay(5000);
      return;
    }

    Serial.println("Adafruit IO conectado.");
  }

  // ----------------------------------------------
  // B. LEER TEMPERATURA Y HUMEDAD
  // ----------------------------------------------

  // Leer la temperatura en grados Celsius.
  float temperatura = dht.readTemperature();

  // Leer la humedad relativa en porcentaje.
  float humedad = dht.readHumidity();

  // Comprobar que el sensor DHT11 haya respondido.
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error al leer el sensor DHT11.");
    delay(2000);
    return;
  }

  // ----------------------------------------------
  // C. MEDIR LA DISTANCIA
  // ----------------------------------------------

  // Mantener TRIG apagado antes de medir.
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  // Enviar un pulso de 10 microsegundos.
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Medir el tiempo que tarda en regresar el eco.
  // Si no llega una respuesta en 30 ms, devuelve 0.
  long duracion = pulseIn(ECHO, HIGH, 30000);

  // Convertir el tiempo en distancia en centimetros.
  // Se divide entre dos porque el sonido va y regresa.
  float distancia = duracion * 0.0343 / 2.0;

  // ----------------------------------------------
  // D. DETERMINAR EL NIVEL DE ALERTA
  // ----------------------------------------------

  // Variable que almacena el nivel correspondiente.
  int nivel;

  // Nivel 1: temperatura menor de 25 grados.
  if (temperatura < 25) {
    nivel = 1;
    Serial.println("NIVEL 1: NORMAL");
  }

  // Nivel 2: temperatura desde 25 hasta menos de 30.
  else if (temperatura < 30) {
    nivel = 2;
    Serial.println("NIVEL 2: PRECAUCION");
  }

  // Nivel 3: temperatura desde 30 hasta menos de 35.
  else if (temperatura < 35) {
    nivel = 3;
    Serial.println("NIVEL 3: ALERTA ALTA");
  }

  // Nivel 4: temperatura igual o superior a 35.
  else {
    nivel = 4;
    Serial.println("NIVEL 4: ALERTA CRITICA");
  }

  // ----------------------------------------------
  // E. MOSTRAR LAS MEDICIONES
  // ----------------------------------------------

  Serial.println("-------------------------");

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" grados C");

  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.println(" %");

  // Mostrar la distancia solamente si se recibio eco.
  if (duracion > 0) {
    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");
  } else {
    Serial.println("Distancia: sin lectura valida");
  }

  Serial.print("Nivel de alerta: ");
  Serial.println(nivel);

  Serial.println("-------------------------");

  // ----------------------------------------------
  // F. ENVIAR LOS DATOS A ADAFRUIT IO
  // ----------------------------------------------

  // Enviar la temperatura al feed correspondiente.
  tempFeed.publish(temperatura);

  // Enviar la humedad al feed correspondiente.
  humFeed.publish(humedad);

  // Enviar la distancia si la lectura fue valida.
  if (duracion > 0) {
    distFeed.publish(distancia);
  }

  // Enviar el nivel de alerta como numero: 1, 2, 3 o 4.
  alertaFeed.publish((int32_t)nivel);

  // ----------------------------------------------
  // G. ESPERAR ANTES DE REPETIR
  // ----------------------------------------------

  // Esperar 5 segundos antes de volver a leer sensores.
  delay(5000);
}
