#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

// ---------- Pinos MCP2515 (HSPI dedicado) ----------
#define CAN_CS    26
#define CAN_INT   25
#define CAN_MOSI  13
#define CAN_MISO  27
#define CAN_SCK   14

// ---------- Objetos ----------
SPIClass hspi(HSPI);
MCP_CAN CAN(CAN_CS, &hspi);

// ---------- Estado ----------
float tensaoAtual = 0.0;
float tensaoAnterior = -1.0;
unsigned long ultimoRequest = 0;
unsigned long ultimaResposta = 0;
bool semResposta = true;

// ---------- CAN ----------
void requisitarTensao() {
  byte req[8] = {0x01, 0, 0, 0, 0, 0, 0, 0};
  CAN.sendMsgBuf(0x7E0, 0, 8, req);
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  // HSPI dedicado pro MCP2515
  hspi.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS);

  // ATENCAO: confira o cristal do MCP2515 (8 ou 16 MHz)
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("Principal: MCP2515 OK");
  } else {
    Serial.println("Principal: ERRO no MCP2515");
    while (1);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);

  Serial.println("Principal pronto (request/response) - modo TERMINAL");
  Serial.println("---------------------------------------------");
}

// ---------- Loop ----------
void loop() {
  // Manda request a cada 200ms
  if (millis() - ultimoRequest >= 200) {
    ultimoRequest = millis();
    requisitarTensao();
  }

  // Escuta respostas
  if (!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len = 0;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);

    if (rxId == 0x7E8 && len >= 3 && rxBuf[0] == 0x01) {
      uint16_t raw = rxBuf[1] | (rxBuf[2] << 8);
      tensaoAtual = raw / 100.0;
      ultimaResposta = millis();
      semResposta = false;

      Serial.print("[OK] Tensao recebida: ");
      Serial.print(tensaoAtual, 2);
      Serial.println(" V");
    }
  }

  // Timeout
  if (millis() - ultimaResposta > 1000) {
    if (!semResposta) {
      Serial.println("[ERRO] Sem resposta do BMS (timeout > 1s)");
    }
    semResposta = true;
  }
}
