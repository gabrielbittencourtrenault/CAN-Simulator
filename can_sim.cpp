#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <array>
#include <algorithm>
#include <cctype>

struct CANFrame {
    uint32_t id;
    uint8_t dlc;
    std::array<uint8_t, 8> data;
};

// ==========================
// Estados simulados
// ==========================
bool bcmEventoA = false;
uint8_t gatewayModo = 0;
uint8_t ivcComando = 0;

// ==========================
// Utilitários
// ==========================
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// ==========================
// Impressão do frame
// ==========================
void printFrame(const CANFrame& frame) {
    std::cout << "ID: 0x" << std::uppercase << std::hex << frame.id << " | DATA: ";

    for (int i = 0; i < frame.dlc; i++) {
        std::cout << std::setw(2) << std::setfill('0')
                  << std::uppercase << std::hex
                  << static_cast<int>(frame.data[i]) << " ";
    }

    std::cout << std::dec << "\n";
}

// ==========================
// Construção dos frames
// ==========================
CANFrame buildBCMFrame() {
    CANFrame frame{};
    frame.id = 0x7CA;
    frame.dlc = 8;
    frame.data.fill(0x00);

    if (bcmEventoA) {
        frame.data[2] |= 0x04; // bit 2 do byte 2
    }

    return frame;
}

CANFrame buildGatewayFrame() {
    CANFrame frame{};
    frame.id = 0x5B3;
    frame.dlc = 8;
    frame.data.fill(0x00);

    frame.data[4] = gatewayModo;

    return frame;
}

CANFrame buildIVCFrame() {
    CANFrame frame{};
    frame.id = 0x612;
    frame.dlc = 8;
    frame.data.fill(0x00);

    frame.data[1] = ivcComando;

    return frame;
}

// ==========================
// Processamento dos frames
// ==========================
void processFrame(const CANFrame& frame) {
    printFrame(frame);

    if (frame.id == 0x7CA) {
        if (frame.data[2] & 0x04) {
            std::cout << ">> BCM: Evento A ATIVO\n";
        } else {
            std::cout << ">> BCM: Evento A INATIVO\n";
        }
    }

    if (frame.id == 0x5B3) {
        std::cout << ">> Gateway modo: " << static_cast<int>(frame.data[4]) << "\n";
    }

    if (frame.id == 0x612) {
        std::cout << ">> IVC comando: " << static_cast<int>(frame.data[1]) << "\n";
    }

    std::cout << "------------------------------\n";
}

// ==========================
// Regras de resposta do sistema
// ==========================
void updateSystemResponses() {
    if (bcmEventoA && ivcComando == 3) {
        gatewayModo = 5;
    } else if (bcmEventoA) {
        gatewayModo = 2;
    } else {
        gatewayModo = 0;
    }
}

// ==========================
// Exibição de status
// ==========================
void showStatus() {
    processFrame(buildBCMFrame());
    processFrame(buildGatewayFrame());
    processFrame(buildIVCFrame());
}

// ==========================
// Tratamento de comandos
// ==========================
void handleCommand(const std::string& rawCmd) {
    std::string cmd = toLower(trim(rawCmd));

    if (cmd == "bcm on") {
        bcmEventoA = true;
        updateSystemResponses();

        std::cout << "Comando aceito: BCM Evento A ON\n";
        processFrame(buildBCMFrame());
        processFrame(buildGatewayFrame());
        return;
    }

    if (cmd == "bcm off") {
        bcmEventoA = false;
        updateSystemResponses();

        std::cout << "Comando aceito: BCM Evento A OFF\n";
        processFrame(buildBCMFrame());
        processFrame(buildGatewayFrame());
        return;
    }

    if (cmd.rfind("gw ", 0) == 0) {
        std::istringstream iss(cmd.substr(3));
        int valor = 0;

        if (iss >> valor && valor >= 0 && valor <= 255) {
            gatewayModo = static_cast<uint8_t>(valor);
            std::cout << "Comando aceito: Gateway atualizado\n";
            processFrame(buildGatewayFrame());
        } else {
            std::cout << "Comando invalido. Exemplo: gw 2\n";
        }
        return;
    }

    if (cmd.rfind("ivc ", 0) == 0) {
        std::istringstream iss(cmd.substr(4));
        int valor = 0;

        if (iss >> valor && valor >= 0 && valor <= 255) {
            ivcComando = static_cast<uint8_t>(valor);
            updateSystemResponses();

            std::cout << "Comando aceito: IVC atualizado\n";
            processFrame(buildIVCFrame());
            processFrame(buildGatewayFrame());
        } else {
            std::cout << "Comando invalido. Exemplo: ivc 3\n";
        }
        return;
    }

    if (cmd == "status") {
        showStatus();
        return;
    }

    if (cmd == "help") {
        std::cout << "Comandos disponiveis:\n";
        std::cout << "  bcm on\n";
        std::cout << "  bcm off\n";
        std::cout << "  gw <0-255>\n";
        std::cout << "  ivc <0-255>\n";
        std::cout << "  status\n";
        std::cout << "  help\n";
        std::cout << "  exit\n";
        std::cout << "------------------------------\n";
        return;
    }

    std::cout << "Comando desconhecido. Digite 'help'.\n";
}

// ==========================
// Main
// ==========================
int main() {
    std::string cmd;

    std::cout << "Simulador CAN em C++ iniciado.\n";
    std::cout << "Comandos disponiveis:\n";
    std::cout << "  bcm on\n";
    std::cout << "  bcm off\n";
    std::cout << "  gw <0-255>\n";
    std::cout << "  ivc <0-255>\n";
    std::cout << "  status\n";
    std::cout << "  help\n";
    std::cout << "  exit\n";
    std::cout << "------------------------------\n";

    while (true) {
        std::cout << ">>> ";
        std::getline(std::cin, cmd);

        if (toLower(trim(cmd)) == "exit") {
            std::cout << "Encerrando simulador.\n";
            break;
        }

        handleCommand(cmd);
    }

    return 0;
}
