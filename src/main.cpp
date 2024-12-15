/*
* Autor: Pedro Henrique Guglielmeti Barbosa
* E-mail: pedrohgb.314(a)gmail.com
*/

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <uri/UriBraces.h>
#include <IRRemote.hpp>
#include <WebServer.h>
#include <Arduino_JSON.h>
#include "definitions.h"

void handle404();
void handleApiCall();


WebServer webServer(80);

void setup(){
  #ifdef DEBUG
    Serial.begin(115200); // Para debug
  #endif

  const uint8_t tSendPin = 4;
  IrSender.begin(tSendPin);

  /* Configuração do WiFi*/
  WiFiManager wifiMan;
  #ifdef AP_TEST
    // Para testar a página de configuração quando o Wi-Fi não está configurado, o programa deve ser compilado com a macro AP_TEST definida.
    // Desta forma, as configurações armazenadas na memória
    wifiMan.resetSettings(); // Remove as configurações de WiFi armazenadas na memória
  #endif

  bool xConnStatus = wifiMan.autoConnect(AP_NAME, AP_PSWD); // TODO: armazenar o nome da AP e a senha na memória SPIFFS; salvar a senha criptografada; permitir que o nome da AP e a senha sejam alteráveis pelo usuário.

  #ifdef DEBUG
    if (xConnStatus)
      // O controlador se conectou à rede WiFi com as credenciais armazenadas na memória SPIFFS
      Serial.println(F("Conexão feita com sucesso."));
    else
      // O controlador não conseguiu se conectar a uma rede WiFi e entrou no modo de Access Point
      Serial.println(F("Falha de conexão."));
  #endif

  const char* requiredHeaders[2] = {"Content-Type", "Content-Length"};

  webServer.collectHeaders(requiredHeaders, sizeof(requiredHeaders)/sizeof(requiredHeaders[0]));

  webServer.onNotFound(handle404);
  webServer.on("/api", handleApiCall);

  webServer.begin();
}

void loop(){
  webServer.handleClient();
  delay(2);
}


void handle404(){
  webServer.send(404, TXT_PLAIN, "Não encontrado");
}

void handleApiCall(){
    #ifdef DEBUG
    // Visualização dos headers da requisição
    for (uint8_t i=0; i<webServer.headers(); i++){
        Serial.print(webServer.headerName(i));
        Serial.print(F("\t:\t"));
        Serial.println(webServer.header(i));
    }
    // Visualização dos argumentos da requisição
    for (uint8_t i = 0; i < webServer.args(); i++){
        Serial.print(webServer.argName(i));
        Serial.print(F("\t:\t"));
        Serial.println(webServer.arg(i));
    }
    #endif

    JSONVar json;
    JSONVar jProtocol;
    JSONVar jLedPin;

    if (webServer.method() != HTTP_POST){
        if (webServer.method() == HTTP_GET){
            // O método GET é implementado pela biblioteca WebServer, mas não é permitido para este handler
            webServer.send(405, APP_JSON, JSON_ERROR("Método não permitido."));
            return;
        }
        // A biblioteca WebServer implementa apenas os métodos GET e POST
        webServer.send(501, APP_JSON, JSON_ERROR("Método não implementado."));
        return;
    }

    if (webServer.header("Content-Type") != APP_JSON){
        // O servidor espera uma mensagem no formato JSON
        webServer.send(415, APP_JSON, JSON_ERROR("Formato não suportado."));
        return;
    }

    for (uint8_t i = 0; i < webServer.args(); i++){
        if (webServer.argName(i) == "plain"){
            json = JSON.parse(webServer.arg(i));

            if (JSON.typeof(json) == "undefined"){
                webServer.send(400, APP_JSON, JSON_ERROR("Erro de formatação JSON."));
                return;
            }
            if (!json.hasOwnProperty("protocolo")){
                webServer.send(422, APP_JSON, JSON_ERROR("Parâmetro \"protocolo\" ausente."));
                return;
            }

            jProtocol = json["protocolo"];

            if (JSON.typeof(jProtocol) != "string"){
                webServer.send(422, APP_JSON, JSON_ERROR("Erro de definição de parâmetro: \"protocolo\"."));
                return;
            }

            const char* c_cptrProtocol = (const char*)jProtocol;

            if (!json.hasOwnProperty("led")){
                webServer.send(422, APP_JSON, JSON_ERROR("Parâmetro \"led\" ausente."));
                return;
            }

            jLedPin = json["led"];

            if (JSON.typeof(jLedPin) != "number"){
                webServer.send(422, APP_JSON, JSON_ERROR("Erro de definição de parâmetro: \"led\"."));
                return;
            }

            #ifdef DEBUG
              Serial.print("Alterando o pino para ");
              Serial.println((uint8_t)jLedPin);
            #endif
            IrSender.setSendPin((uint8_t)jLedPin);  // Alteração do pino a ser acionado

            /* Comando sem protocolo definido (sinal bruto) */
            if (strcmp(c_cptrProtocol, "raw") == 0){
                if (!json.hasOwnProperty("frequencia") || !json.hasOwnProperty("dados")){
                    webServer.send(422, APP_JSON, JSON_ERROR("Parâmetros ausentes."));
                    return;
                }
                
                JSONVar jFreq = json["frequencia"];
                JSONVar jData = json["dados"];

                if (JSON.typeof(jFreq) != "number" || JSON.typeof(jData) != "array"){
                    webServer.send(422, APP_JSON, JSON_ERROR("Erro de definição de parâmetros."));
                    return;
                }
                
                uint8_t ui8FreqKhz = (uint8_t)jFreq;

                // Aqui temos alguns problemas a considerar para melhorias futuras:
                // - Não está sendo feita a verificação dos tipo dos valores dentro do array. Um array JSON pode conter valores de qualquer tipo. Aqui está simplesmente sendo feito o casting dos valores para uint8_t ou uint16_t, o que pode gerar um sinal não esperado pelo usuário.
                // - Não está sendo considerado o limite de memória do dispositivo. Uma requisição poderia chegar com um array com mais elementos do que seria possível alocar na memória.
                uint16_t a_ui16Data[jData.length()];

                for (uint8_t i=0; i<jData.length(); i++){
                    a_ui16Data[i] = (uint16_t)jData[i];
                }

                IrSender.sendRaw(a_ui16Data, sizeof(a_ui16Data) / sizeof(a_ui16Data[0]), 38);
            }

                /* Comandos com protocolo definido */
                // No momento são suportados os protocolos NEC, NEC1, NEC2, RC5, RC6 e Panasonic
                // Sinais que utilizem outros protocolos devem ser enviados no formato bruto
            else{
                if (!json.hasOwnProperty("device") || !json.hasOwnProperty("subdevice") || !json.hasOwnProperty("function")){
                    webServer.send(422, APP_JSON, JSON_ERROR("Parâmetros ausentes."));
                    return;
                }

                JSONVar jDevice = json["device"];
                JSONVar jSubdevice = json["subdevice"];
                JSONVar jFunction = json["function"];
                uint16_t ui16Address;

                // Aqui não é feita nenhuma verificação quanto à natureza das variáveis numéricas (tipo, tamanho).
                // Se elas forem numéricas, apenas será feito o casting para tipos compatíveis com os argumentos das funções de envio e nenhum erro será retornado pela API.
                // Por exemplo, caso "device" tenha originalmente o valor 4294967295 (0xffffffff), ele se tornará 65535 (0xffff). Caso "device" tenha originalmente o valor 2.5, ele se tornará 2.
                // Esta verificação pode ser feita num implementação futura.
                if (JSON.typeof(jDevice) != "number" || JSON.typeof(jSubdevice) != "number" || JSON.typeof(jFunction) != "number"){
                    webServer.send(422, APP_JSON, JSON_ERROR("Erro de definição de parâmetro. Tipo não permitido."));
                    return;
                }

                ui16Address = ((uint16_t)jDevice << 8) | (uint16_t)jSubdevice;   // Transformação dos parâmetros "device" e "subdevice" em um único byte ("address"), adotado pela biblioteca IRRemote

                if IS_NEC(c_cptrProtocol){  // Verificando se o protocolo é NEC, NEC1 ou NEC2
                    IrSender.sendNEC(ui16Address, (uint16_t)jFunction, 0);
                }
                else if (strcmp(c_cptrProtocol, "RC5") == 0)
                    IrSender.sendRC5(ui16Address, (uint16_t)jFunction, 0, true);
                else if (strcmp(c_cptrProtocol, "RC6") == 0)
                    IrSender.sendRC6(ui16Address, (uint16_t)jFunction, 0, true);
                else if (strcmp(c_cptrProtocol, "Panasonic") == 0)
                    IrSender.sendPanasonic(ui16Address, (uint16_t)jFunction, 0);
                else{
                    webServer.send(422, APP_JSON, JSON_ERROR("Protocolo não suportado."));
                    return;
                }
            }
            break;  // Só estamos interessados no parâmetro "plain", que é o que deve conter a mensagem em formato JSON, então após encontrá-lo, podemos deixar o loop
        }
    }

    webServer.send(200, APP_JSON, JSON_SUCCESS("Comando enviado."));  // Se a execução chegou até este ponto, não houve nenhuma falha na identificação dos parâmetros da mensagem e o sinal IR foi emitido
}
/* EOF */