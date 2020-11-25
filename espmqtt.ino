
//Programa: TCC-NodeMCU
//Autor: Roger Peixoto Duarte
 
#include <ESP8266WiFi.h> 
#include <PubSubClient.h> 


//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_LED_CONTROL "esp/led/control" //Topico usado para controlar o LED
#define TOPICO_LED_SEND_STATUS "esp/led/sendStatus" //Topico utilizado para enviar o status do led
#define TOPICO_LED_STATUS_GET "esp/led/getStatus" //Topico utilizado para solicitar o status do Led
#define TOPICO_PRESENCE   "esp/presence"    //Topico utilizado para enviar presença do dispositivo
#define TOPICO_CONNECTION_STATUS "esp/connection/sendStatus" //Topico usado para enviar o estado da conexão da ESP com a rede
#define TOPICO_PWMLED_CONTROL "esp/pwmLed/control" // Topico utilizado para controlar o valor de tensao em uma porta analógica
#define TOPICO_PWMLED_SEND_STATUS "esp/pwmLed/sendStatus" //Topico utilizado para enviar o status do led controlado por pwm
#define TOPICO_PWMLED_GET_STATUS "esp/pwmLed/getStatus" //Topico utilizado para solicitar o valor atual de tensão do led controlado por pwm
#define TOPICO_BUTTON_STATUS "esp/button/sendStatus" //Topico usado para enviar o status do "botão"
#define TOPICO_POT_GET_CONTROL "esp/pot/getControl" //Topico usado para solicitar o estado do controle do potenciometro
#define TOPICO_POT_SEND_CONTROL "esp/pot/sendControl" // Topico usado para enviar o estado do controle do potenciometro
#define TOPICO_POT_SET_CONTROL "esp/pot/setControl" //Topico usado para alterar o estado do controle do potenciometro
#define TOPICO_POT_STATUS "esp/pot/sendStatus" //Topico usado para ativar ou desativar o controle do potenciometro
#define TOPICO_OVERALL_GET_STATUS "esp/overall/getStatus" // Topico usado para solicitar o estado geral das portas objetos de teste
#define TOPICO_OVERALL_STATUS_SEND "esp/overall/sendStatus" // Topico usado para enviar o estado geral das portas

#define ID_MQTT  "HomeAut"     //id mqtt (para identificação de sessão)

                                
 
//defines - mapeamento de pinos do NodeMCU
#define D0    16
#define D1    5
#define D2    4
#define D3    0
#define D4    2
#define D5    14
#define D6    12
#define D7    13
#define D8    15
#define D9    3
#define D10   1

int pwmLed = D4; //Led regulado por PWM através do TOPICO_PWMLED_CONTROL
int statusLed = D3; // Led usado para testes de status
int button = D2; // "Botão" utilizado para testes de status
String potMode = "false"; // Status do modo de escuta de mudanças no potenciometro


// WIFI
const char* SSID = ""; 
const char* PASSWORD = ""; 
  
// MQTT
const char* BROKER_MQTT = "";
int BROKER_PORT = 1883; 
 
 
//Variáveis e objetos globais
WiFiClient espClient; 
PubSubClient MQTT(espClient); 
  
//Prototypes
void initSerial(); 
void initWiFi(); 
void initMQTT(); 
void reconectWiFi(); 
void mqtt_callback(char* topic, byte* payload, unsigned int length); 
void verifyConnections(void); 
void InitOutput(void);
void verifyPotReading(void);
void verifyButton(void); 
void sendLedStatus(void); 
void sendPotStatus(void);
void sendButtonStatus(void); 
void sendConnectionStatus(void); 
void sendStatus(void); 

/* 
 *  Implementações das funções
 */
void setup() 
{
    //inicializações:
    InitOutput();
    initSerial();
    initWiFi();
    initMQTT();
}
  
//Função: inicializa comunicação serial com baudrate 115200 (para fins de monitorar no terminal serial 
//        o que está acontecendo.
//Parâmetros: nenhum
//Retorno: nenhum
void initSerial() 
{
    Serial.begin(115200);
}
 
//Função: inicializa e conecta-se na rede WI-FI desejada
//Parâmetros: nenhum
//Retorno: nenhum
void initWiFi() 
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
     
    reconectWiFi();
}
  
//Função: inicializa parâmetros de conexão MQTT(endereço do 
//        broker, porta e configura função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   
    MQTT.setCallback(mqtt_callback);          
}
  
//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos inscritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;

    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    
    String receivedTopic = String(topic);
    Serial.print(receivedTopic);

    //Toma ação dependendo da string recebida:
    if(receivedTopic.equals(TOPICO_POT_SET_CONTROL)) { 
      potMode=msg;
    }
    if(receivedTopic.equals(TOPICO_PWMLED_CONTROL)) { 
      controlPwmLed(payload, length);
    }
    if(receivedTopic.equals(TOPICO_OVERALL_GET_STATUS)) { 
      sendStatus();
    }
    if(receivedTopic.equals(TOPICO_LED_CONTROL)) { 
      controlStatusLed(msg);
    }     
}
  
//Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
//        em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
//Parâmetros: nenhum
//Retorno: nenhum
void reconnectMQTT() {
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT,NULL,NULL,TOPICO_CONNECTION_STATUS, 0, false, "Disconnected")) 
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_LED_CONTROL);
            MQTT.subscribe(TOPICO_OVERALL_GET_STATUS);
            MQTT.subscribe(TOPICO_PWMLED_CONTROL);
            MQTT.subscribe(TOPICO_POT_SET_CONTROL);
            MQTT.publish(TOPICO_PRESENCE, "ESP CONECTADA");
            sendConnectionStatus();
        } 
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}
  
//Função: reconecta-se ao WiFi
//Parâmetros: nenhum
//Retorno: nenhum
void reconectWiFi() 
{
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD);
     
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(100);
        Serial.print(".");
    }
   
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}
 
//Função: verifica o estado das conexões WiFI e ao broker MQTT. 
//        Em caso de desconexão (qualquer uma das duas), a conexão
//        é refeita.
//Parâmetros: nenhum
//Retorno: nenhum
void verifyConnections(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT();
     
     reconectWiFi();
}
 
//Função: inicializa as saídas
//Parâmetros: nenhum
//Retorno: nenhum
void InitOutput(void)
{
    pinMode(pwmLed, OUTPUT);
    pinMode(A0, INPUT); 
    pinMode(statusLed, OUTPUT);
    pinMode(button, INPUT_PULLUP);    
}

//Função: Controla o estado do Led presente na saída digital
//Parâmetros: nenhum
//Retorno: nenhum

void controlStatusLed(String msg) {
    if (msg.equals("0")) {
      digitalWrite(statusLed, LOW);
      Serial.print(digitalRead(statusLed));
    }
    if (msg.equals("1")){
      digitalWrite(statusLed, HIGH);
      Serial.print(digitalRead(statusLed));
    }
    sendLedStatus();
}

//Função: Função que trata o valor
//de pwm recebido e ajusta o brilho do led;
//Parâmetros: a mensagem em byte e o seu tamanho em inteiro
//Retorno: nenhum
void controlPwmLed(byte* payload, unsigned int length) {
    char format[16];
    snprintf(format, sizeof format, "%%%ud", length);
    int payload_value = 0;
    if (sscanf((const char *) payload, format, &payload_value) == 1){
      analogWrite(pwmLed, payload_value);
    } else ;
}

//Função: Verifica o estado do botão e o envia;
//Parâmetros: nenhum
//Retorno: nenhum
void verifyButton(void) {
  if(digitalRead(button) == LOW) {
    sendButtonStatus();
    while(digitalRead(button) == LOW) {
      delay(100);    
    }
    sendButtonStatus();
  }
}

//Função: Verifica o modo de escuta do potenciômetro
//        e envia o valor atual da tensão regulada por ele
//Parâmetros: nenhum
//Retorno: nenhum
void verifyPotReading(void){
  if(potMode.equals("true")){
    Serial.print("ADC Value: ");Serial.println(analogRead(A0));
    sendPotStatus();
    delay(500);
  }
}


//Função: Envia o estado do led ligado na porta digital;
//Parâmetros: nenhum
//Retorno: nenhum
void sendLedStatus(void) {
  int ledStatus = digitalRead(statusLed);
  char cstr[sizeof(ledStatus)*8+1];
  itoa(ledStatus, cstr, 10);
  MQTT.publish(TOPICO_LED_SEND_STATUS, cstr);
}

//Função: Envia o valor da tensão regulada pelo potenciômetro;
//Parâmetros: nenhum
//Retorno: nenhum
void sendPotStatus(void) {
  int potValue = analogRead(A0);
  char cstr[sizeof(potValue)*8+1];
  itoa(potValue, cstr, 10);
  MQTT.publish(TOPICO_POT_STATUS, cstr);
}

//Função: Envia o estado do modo de escuta do potenciômetro;
//Parâmetros: nenhum
//Retorno: nenhum
void sendPotControl(void) {
  char potModeCopy[20];
  potMode.toCharArray(potModeCopy, 20);
  MQTT.publish(TOPICO_POT_SEND_CONTROL, potModeCopy);
}

//Função: Envia o estado do botão;
//Parâmetros: nenhum
//Retorno: nenhum
void sendButtonStatus(void) {
  int buttonStatus = !digitalRead(button);
  char cstr[sizeof(buttonStatus*8+1)];
  itoa(buttonStatus, cstr, 10);
  MQTT.publish(TOPICO_BUTTON_STATUS, cstr);
}

//Função: Envia o estado da conexão da ESP;
//Parâmetros: nenhum
//Retorno: nenhum
void sendConnectionStatus(void) {
  MQTT.publish(TOPICO_CONNECTION_STATUS, "Connected");
}

//Função: Envia o estadao geral das saídas e da conexão;
//Parâmetros: nenhum
//Retorno: nenhum
void sendStatus(void) {
  sendConnectionStatus();
  sendLedStatus();
  sendPotStatus();
  sendPotControl();
  sendButtonStatus();
}

 
//programa principal
void loop() 
{   

    verifyConnections();
    verifyButton();
    verifyPotReading();
    MQTT.loop();
}
