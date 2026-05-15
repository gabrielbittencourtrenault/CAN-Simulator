#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

// ---------- Pinos MCP2515 ----------
#define CAN_CS    10
#define CAN_MISO  11   // MCP SO
#define CAN_MOSI  12   // MCP SI
#define CAN_SCK   13
#define CAN_INT   14

// ---------- Objeto ----------
MCP_CAN CAN(CAN_CS);

// ---------- Estado ----------
float tensao = 48.00;
bool subindo = true;
unsigned long ultimaAtualizacao = 0;

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  // ESP32-S3 precisa configurar pinos SPI
  SPI.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS);

  // ATENCAO: confira o cristal do MCP2515 (8 ou 16 MHz)
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("BMS Sim: MCP2515 OK");
  } else {
    Serial.println("BMS Sim: ERRO no MCP2515");
    while (1);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);

  Serial.println("BMS Simulador pronto (request/response)");
}

// ---------- Loop ----------
void loop() {
  // Atualiza tensao simulada (48-50V em onda triangular)
  if (millis() - ultimaAtualizacao >= 100) {
    ultimaAtualizacao = millis();
    if (subindo) {
      tensao += 0.05;
      if (tensao >= 50.00) subindo = false;
    } else {
      tensao -= 0.05;
      if (tensao <= 48.00) subindo = true;
    }
  }

  // Escuta requests
  if (!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len = 0;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);

    if (rxId == 0x7E0 && len >= 1) {
      byte pid = rxBuf[0];
      byte resp[8] = {0};
      resp[0] = pid;

      switch (pid) {
        case 0x01: {  // tensao
          uint16_t raw = (uint16_t)(tensao * 100);
          resp[1] = raw & 0xFF;
          resp[2] = (raw >> 8) & 0xFF;
          break;
        }
        default:
          resp[1] = 0xFF;
          break;
      }

      byte sndStat = CAN.sendMsgBuf(0x7E8, 0, 8, resp);
      if (sndStat == CAN_OK) {
        Serial.print("Respondi PID 0x");
        Serial.print(pid, HEX);
        Serial.print(" | tensao=");
        Serial.print(tensao, 2);
        Serial.println(" V");
      } else {
        Serial.println("Erro ao enviar");
      }
    }
  }
}
