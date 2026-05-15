#include <SPI.h>
#include <mcp_can.h>

// Pinos do ESP32-S3
#define CAN_CS    10
#define CAN_INT   14
#define CAN_MOSI  11
#define CAN_MISO  13
#define CAN_SCK   12

MCP_CAN CAN(CAN_CS);

// Tensão simulada
float tensao = 48.00;
bool subindo = true;
unsigned long ultimaAtualizacao = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  
  // SPI customizado pro ESP32-S3
  SPI.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS);
  
  // ATENÇÃO: confere o cristal do seu módulo (8 ou 16 MHz)
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("BMS Sim: MCP2515 OK");
  } else {
    Serial.println("BMS Sim: ERRO no MCP2515");
    while (1);
  }
  
  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);
  
  Serial.println("BMS Simulador pronto (modo request/response)");
}

void loop() {
  // Atualiza tensão simulada continuamente (varia 48-50V)
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
  
  // Escuta requests no barramento
  if (!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len = 0;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);
    
    // Request vem com ID 0x7E0
    if (rxId == 0x7E0 && len >= 1) {
      byte pid = rxBuf[0];
      byte resp[8] = {0};
      resp[0] = pid;  // ecoa o PID na resposta
      
      switch (pid) {
        case 0x01: {  // PID 0x01 = tensão
          uint16_t raw = (uint16_t)(tensao * 100);
          resp[1] = raw & 0xFF;         // byte baixo
          resp[2] = (raw >> 8) & 0xFF;  // byte alto
          break;
        }
        // PIDs futuros:
        // case 0x02: SOC
        // case 0x03: SOH
        default:
          resp[1] = 0xFF;  // erro
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
        Serial.println("Erro ao responder");
      }
    }
  }
}
