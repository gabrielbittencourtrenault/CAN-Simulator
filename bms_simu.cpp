#include <Arduino.h>
#include <SPI.h>
#include <mcp_can.h>

// ---------- Pinos MCP2515 ----------
#define CAN_CS    10
#define CAN_MISO  13   // MCP SO
#define CAN_MOSI  11   // MCP SI
#define CAN_SCK   12
#define CAN_INT   14

// ---------- Objeto ----------
SPIClass fspi2(FSPI);
MCP_CAN CAN(&fspi2, CAN_CS);

// Definição IDs CAN
const unsigned long ID_WAKEUP   = 0x100;
const unsigned long ID_WAKEUPON = 0x101;
const unsigned long ID_SOC      = 0x200;
const unsigned long ID_SOCR     = 0X201;

// Frames
byte FRAME_WAKEUP [8] = {0XAA, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00};
byte FRAME_WAKEUPON [8] = {0XAB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte FRAME_SOC [8] = {0XBB, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00};
byte FRAME_SOCR [8] = {0XBC, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00, 0X00};

// ---------- Estado ----------

void sendwakeupon() {
  CAN.sendMsgBuf(ID_WAKEUPON, 0, 8, FRAME_WAKEUPON);
}

void sendsocr() {
  CAN.sendMsgBuf(ID_SOCR, 0, 8, FRAME_SOCR);
}

void checkrequestwakeup() {

  Serial.print("INT: ");
  Serial.println(digitalRead(CAN_INT));

    if(!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);

    Serial.print("Simulador recebeu ID: 0x");
        Serial.println(rxId, HEX);
  
  if (rxId == ID_WAKEUP && rxBuf[0] == 0xAA) {
    Serial.println("Enviando WAKEUPON...");
    sendwakeupon(); 
  }
 }
}

 void checkrequestsoc() {
  if(!digitalRead(CAN_INT)) {
    long unsigned int rxId;
    byte len;
    byte rxBuf[8];
    CAN.readMsgBuf(&rxId, &len, rxBuf);
  
  if (rxId == ID_SOC && rxBuf[0] == 0xBB)
    sendsocr();
 }
 }

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(500);

  // ESP32-S3 precisa configurar pinos SPI
  fspi2.begin(CAN_SCK, CAN_MISO, CAN_MOSI, CAN_CS);

  // ATENCAO: confira o cristal do MCP2515 (8 ou 16 MHz)
  if (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("BMS: MCP2515 OK");
  } else {
    Serial.println("BMS: ERRO no MCP2515");
    while (1);
  }

  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT_PULLUP);

  Serial.println("BMS Simulador pronto (response)");
}

// ---------- Loop ----------
void loop() {
  checkrequestwakeup();
  checkrequestsoc();
}


