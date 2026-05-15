#include <SPI.h>
#include <mcp_can.h>
#include <TFT_eSPI.h>

#define CAN_CS   5
#define CAN_INT  22

MCP_CAN CAN(CAN_CS);
TFT_eSPI tft = TFT_eSPI();

float tensaoAtual = 0.0;
float tensaoAnterior = -1.0;
unsigned long ultimoRequest = 0;
unsigned long ultimaResposta = 0;
bool semResposta = false;

void desenharTelaInicial() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(20, 20);
  tft.print("BMS Monitor");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(20, 90);
  tft.print("Tensao da bateria:");
}

void atualizarLCD(float tensao, bool valido) {
  tft.fillRect(20, 130, 440, 70, TFT_BLACK);
  tft.setTextSize(6);
  tft.setCursor(20, 130);
  
  if (valido) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("%.2f V", tensao);
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("-- . -- V");
  }
}

void requisitarTensao() {
  byte req[8] = {0x01, 0, 0, 0, 0, 0, 0, 0};  // PID 0x01 = tensão
  CAN.sendMsgBuf(0x7E0, 0, 8, req);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // Inicializa LCD
  tft.init();
  tft.setRotation(1);
  desenharTelaInicial();
  
  // Inicializa CAN
  // ATENÇÃO: confere o cristal do seu módulo (8 ou 16 MHz)
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("Principal: MCP2515 OK");
  } else {
    Serial.println("Principal: ERRO no MCP2515");
    tft.setCursor(20, 230);
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.print("ERRO MCP2515");
    while (1);
  }
  
  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);
  
  Serial.println("Principal pronto (modo request/response)");
}

void loop() {
  // Manda request de tensão a cada 200ms
  if (millis() - ultimoRequest >= 200) {
    ultimoRequest = millis();
    requisitarTensao();
  }
  
  // Escuta resposta
  if (!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len = 0;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);
    
    // Resposta vem com ID 0x7E8
    if (rxId == 0x7E8 && len >= 3 && rxBuf[0] == 0x01) {
      uint16_t raw = rxBuf[1] | (rxBuf[2] << 8);
      tensaoAtual = raw / 100.0;
      ultimaResposta = millis();
      semResposta = false;
      
      Serial.print("Tensao recebida: ");
      Serial.print(tensaoAtual, 2);
      Serial.println(" V");
    }
  }
  
  // Detecta timeout (sem resposta há mais de 1 segundo)
  if (millis() - ultimaResposta > 1000) {
    semResposta = true;
  }
  
  // Atualiza LCD só quando muda
  if (abs(tensaoAtual - tensaoAnterior) >= 0.01 || semResposta) {
    atualizarLCD(tensaoAtual, !semResposta);
    tensaoAnterior = tensaoAtual;
  }
}
