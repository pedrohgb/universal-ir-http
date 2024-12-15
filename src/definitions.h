#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#define DEBUG   // Este define faz com que o programa seja compilado de forma a enviar mensagens pela interface serial para fins de teste e debug
#define DISABLE_CODE_FOR_RECEIVER
#define NO_LED_FEEDBACK_CODE
#define EXCLUDE_UNIVERSAL_PROTOCOLS

#ifdef IR_SEND_PIN
    #undef IR_SEND_PIN      // Remoção da definição de IR_SEND_PIN, de forma que o pino por onde será enviado o sinal IR possa ser alterado no tempo de execução
#endif /* IR_SEND_PIN */

/* Macros */


#define AP_NAME "Controle Remoto"
#define AP_PSWD "123456789"

/* Content-Type */
#define TXT_HTML  "text/html"
#define TXT_PLAIN "text/plain"
#define APP_JSON  "application/json"

// As macros JSON_SUCCESS e JSON_ERROR só são válidas se "msg" for uma string literal
#define JSON_SUCCESS(msg) "{\"erro\":false,\"mensagem\":\"" msg "\"}"
#define JSON_ERROR(msg) "{\"erro\":true,\"mensagem\":\"" msg "\"}"

#define IS_NEC(str)   (strcmp(str, "NEC") == 0 || strcmp(str, "NEC1") == 0 || strcmp(str, "NEC2") == 0) // Macro para verificar se str indica algum protocolo da família NEC; definido para melhorar a legibilidade do código fonte


#endif /* DEFINITIONS_H */