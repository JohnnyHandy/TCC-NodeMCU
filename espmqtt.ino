
//Programa: NodeMCU e MQTT - Controle e Monitoramento IoT
//Autor: Roger Peixoto Duarte
 
#include <ESP8266WiFi.h> // Importa a Biblioteca ESP8266WiFi
#include <PubSubClient.h> // Importa a Biblioteca PubSubClient


//defines:
//defines de id mqtt e tópicos para publicação e subscribe
#define TOPICO_LED_CONTROL "esp/led/control" //Topico usado para controlar o LED
#define TOPICO_LED_STATUS "esp/led/status" //Topico utilizado para enviar o status do led
#define TOPICO_LED_STATUS_GET "esp/led/status/get" //Topico utilizado para solicitar o status do Led
#define TOPICO_PRESENCE   "presence"    //Topico utilizado para enviar presença do dispositivo
#define TOPICO_CONNECTION_STATUS "esp/connection_status" //Topico usado para enviar o estado da conexão da ESP com a rede
#define TOPICO_ANALOG_WRITE "esp/led/analogwrite" // Topico utilizado para controlar o valor de tensao em uma porta analógica
#define TOPICO_BUTTON_STATUS "esp/button/status" //Topico usado para enviar o status do "botão"
#define TOPICO_POT_CONTROL "esp/pot/control" //Topico usado para enviar o estado do potenciometro
#define TOPICO_POT_STATUS "esp/pot/status" //Topico usado para ativar ou desativar o controle do potenciometro

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

int pwmLed = D4; //Led regulado por PWM através do TOPICO_ANALOG_WRITE
int statusLed = D3; // Led usado para testes de status
int button = D2; // "Botão" utilizado para testes de status
String potMode = "false"; // Status do modo de escuta de mudanças no potenciometro


// WIFI
const char* SSID = "VIVOFIBRA-98C5"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "apt1012020"; // Senha da rede WI-FI que deseja se conectar
  
// MQTT
const char* BROKER_MQTT = "192.168.15.12"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT
 
 
//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient
  
//Prototypes
void initSerial(); //Configura o Baudrate
void initWiFi(); // Configura conexão com wifi
void initMQTT(); // Configura conexão MQTT
void reconectWiFi(); // Usado para reconectar com o wi-fi
void mqtt_callback(char* topic, byte* payload, unsigned int length); // Prototipo de função que escuta e identifica mensagens MQTT
void VerificaConexoesWiFIEMQTT(void); // Verificação de status de conexões Wifi e MQTT
void InitOutput(void);
void verificaLeituraPot(void); //Verifica leitura do potenciometro caso o modo de escuta esteja ativado
void verificaLeituraBotao(void); // Verifica status do "botão"
void enviaStatusLed(void); //Envia o estado do led de teste de status
void enviaStatusPot(void); //Envia o valor atual da conversão analógica-digital da tensao da entrada analogica ligada ao potenciometro
void enviaStatusBut(void); //Envia o estado do botão de teste
void enviaStatusConn(void); //Envia estado da conexão
void enviaStatus(void); //Envia status geral dos objetos de teste na esp

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
//        broker, porta e seta função de callback)
//Parâmetros: nenhum
//Retorno: nenhum
void initMQTT() 
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}
  
//Função: função de callback 
//        esta função é chamada toda vez que uma informação de 
//        um dos tópicos subescritos chega)
//Parâmetros: nenhum
//Retorno: nenhum
void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
    String msg;
 
    //obtem a string do payload recebido
    for(int i = 0; i < length; i++) 
    {
       char c = (char)payload[i];
       msg += c;
    }
    
    String receivedTopic = String(topic); //Converteo topico recebido em uma string
    Serial.print(receivedTopic);

    //Toma ação dependendo da string recebida:
    
    if(receivedTopic.equals(TOPICO_POT_CONTROL)) { //Ativa o modo de escuta do potenciometro
      potMode=msg;
    }
    if(receivedTopic.equals(TOPICO_ANALOG_WRITE)) { //Envia valor analógico à porta de entrada analógica
      char format[16];
      snprintf(format, sizeof format, "%%%ud", length);

      // Converte o valor contigo na mensagem para um valor inteiro
      int payload_value = 0;
      if (sscanf((const char *) payload, format, &payload_value) == 1){
        analogWrite(D4, payload_value);
      } else ;
    }
    if(receivedTopic.equals(TOPICO_LED_STATUS_GET)) { //Envia o status geral da esp
      enviaStatus();
    }
        
    if(receivedTopic.equals(TOPICO_LED_CONTROL)) { //Muda o estado do LED de acordo com a mensagem recebida
      if (msg.equals("0")) {
        digitalWrite(statusLed, LOW);
        Serial.print(digitalRead(statusLed));
        MQTT.publish(TOPICO_LED_STATUS, "0");
      }
      if (msg.equals("1")){
        digitalWrite(statusLed, HIGH);
        Serial.print(digitalRead(statusLed));
        MQTT.publish(TOPICO_LED_STATUS, "1");
      }
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
            MQTT.subscribe(TOPICO_LED_STATUS_GET);
            MQTT.subscribe(TOPICO_ANALOG_WRITE);
            MQTT.subscribe(TOPICO_POT_CONTROL);
            MQTT.publish(TOPICO_PRESENCE, "ESP CONECTADA");
            MQTT.publish(TOPICO_CONNECTION_STATUS, "Connected");
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
    //se já está conectado a rede WI-FI, nada é feito. 
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
         
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
     
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
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected()) 
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita
     
     reconectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}
 
//Função: inicializa o output em nível lógico baixo
//Parâmetros: nenhum
//Retorno: nenhum
void InitOutput(void)
{
    //IMPORTANTE: o Led já contido na placa é acionado com lógica invertida (ou seja,
    //enviar HIGH para o output faz o Led apagar / enviar LOW faz o Led acender)
    pinMode(pwmLed, OUTPUT);
    pinMode(A0, INPUT); 
    pinMode(statusLed, OUTPUT);
    pinMode(button, INPUT_PULLUP);    
}

void verificaLeituraBotao(void) {
  if(digitalRead(button) == LOW) {
   MQTT.publish(TOPICO_BUTTON_STATUS, "1");
    // Wait for the button to be released
    while(digitalRead(button) == LOW) {
      delay(100);    
    }

    // Publish a button pushed message to a topic
    MQTT.publish(TOPICO_BUTTON_STATUS, "0");

  }
}

void verificaLeituraPot(void){
  if(potMode.equals("true")){
    Serial.print("ADC Value: ");Serial.println(analogRead(A0));
    int num = analogRead(A0);
    char cstr[16];
    itoa(num, cstr, 10);
    MQTT.publish(TOPICO_POT_STATUS, cstr);
    delay(500);
  }
}

void enviaStatusLed(void) {
   int num = digitalRead(statusLed);
  char cstr[16];
  itoa(num, cstr, 10);
  MQTT.publish(TOPICO_LED_STATUS, cstr);
}

void enviaStatusPot(void) {
  int num2 = analogRead(A0);
  char cstr2[16];
  itoa(num2, cstr2, 10);
  MQTT.publish(TOPICO_POT_STATUS, cstr2);
}

void enviaStatusBut(void) {
  int num3 = !digitalRead(button);
  char cstr3[16];
  itoa(num3, cstr3, 10);
  MQTT.publish(TOPICO_BUTTON_STATUS, cstr3);
}

void enviaStatusConn(void) {
  MQTT.publish(TOPICO_CONNECTION_STATUS, "Connected");
}

void enviaStatus(void) {
  enviaStatusConn();
  enviaStatusLed();
  enviaStatusPot();
  enviaStatusBut(); 
}

 
//programa principal
void loop() 
{   
    //garante funcionamento das conexões WiFi e ao broker MQTT
    VerificaConexoesWiFIEMQTT();
    verificaLeituraBotao();
    verificaLeituraPot();
    //keep-alive da comunicação com broker MQTT
    MQTT.loop();
}
