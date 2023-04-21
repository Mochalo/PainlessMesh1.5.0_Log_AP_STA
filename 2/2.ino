#include <painlessMesh.h>
#include <Preferences.h>

constexpr char MESH_PREFIX[] = "your_mesh_ssid";
constexpr char MESH_PASSWORD[] = "your_mesh_password";
constexpr uint16_t MESH_PORT = 5555;

Preferences preferences;
Scheduler userScheduler;
painlessMesh mesh;

const int maxLogsPercentage = 30;
int maxLogsCount = 0;
int currentLogIndex = 0;

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("log: Received from %u msg=%s\n", from, msg.c_str());
  String logEntry = "RECEIVED: from " + String(from) + " msg=" + msg;
  saveLog(logEntry);
  Serial.println("Received message log saved");
}

void newConnectionCallback(uint32_t nodeId, bool isAP) {
  uint32_t current_node_id = mesh.getNodeId();
  Serial.printf("log: New %s Connection, nodeId = %u, connected to %u\n", isAP ? "AP" : "STA", nodeId, current_node_id);
  String logEntry = "CONNECTION: New " + String(isAP ? "AP" : "STA") + " Connection, nodeId = " + String(nodeId) + ", connected to " + String(current_node_id);
  saveLog(logEntry);
  Serial.println("New connection log saved");
}

void printFunction() {
  Serial.println("\nCurrent Node List:");
  SimpleList<uint32_t>::iterator nodeIter = mesh.getNodeList().begin();
  while (nodeIter != mesh.getNodeList().end()) {
    String nodeType = (*nodeIter & 1) ? "AP" : "STA";
    Serial.printf("Node ID: %u Type: %s\n", *nodeIter, nodeType.c_str());
    ++nodeIter;
  }
  printAllLogs();
  //PrintWifiStatus();
};

Task printTableTask(30000, TASK_FOREVER, printFunction);

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | APPLICATION | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection([](uint32_t nodeId) {
    newConnectionCallback(nodeId, true);
  });

  preferences.begin("logNamespace", false);

  maxLogsCount = (ESP.getFlashChipSize() / 100) * maxLogsPercentage / sizeof(String);

  userScheduler.addTask(printTableTask);
  printTableTask.enable();
}

void loop() {
  mesh.update();
}

void saveLog(const String &logEntry) {
  Serial.println("Saving log: " + logEntry);
  preferences.putString(String(currentLogIndex).c_str(), logEntry);
  currentLogIndex = (currentLogIndex + 1) % maxLogsCount;
}

String getLog(int index) {
  return preferences.getString(String(index).c_str(), "");
}
void printAllLogs() {
  int logCount = 0;
  for (int i = 0; i < maxLogsCount; ++i) {
    String logEntry = getLog(i);
    if (!logEntry.isEmpty()) {
      String logType;
      if (logEntry.startsWith("RECEIVED")) {
        logType = "RECEIVED";
      } else if (logEntry.startsWith("CONNECTION: New AP")) {
        logType = "AP CONNECTION";
      } else if (logEntry.startsWith("CONNECTION: New STA")) {
        logType = "STA CONNECTION";
      }
      Serial.printf("[%s] %s\n", logType.c_str(), logEntry.c_str());
      logCount++;
    }
  }

  // Вывод информации о пространстве, используемом для хранения логов
  size_t totalSpace = maxLogsCount * sizeof(String);
  size_t usedSpace = logCount * sizeof(String);
  size_t freeSpace = totalSpace - usedSpace;
  float usedPercentage = (float)usedSpace / totalSpace * 100;

  Serial.printf("\nLog space:\n");
  Serial.printf("Total: %u bytes\n", totalSpace);
  Serial.printf("Free: %u bytes\n", freeSpace);
  Serial.printf("Used: %u bytes\n", usedSpace);
  Serial.printf("Used percentage: %.2f%%\n", usedPercentage);
}


void PrintWifiStatus() {
  String ssid = WiFi.SSID().c_str();
  IPAddress apIP = WiFi.softAPIP();
  IPAddress IP = WiFi.localIP();
  bool wifiConnected = WiFi.isConnected();
  int apClients = WiFi.softAPgetStationNum();
  int localClients = apClients + (wifiConnected ? 1 : 0);
  int apTxPower = WiFi.getTxPower();
  int rssi = WiFi.RSSI();
  int meshClients = mesh.getNodeList().size();
  uint32_t thisNodeId = mesh.getNodeId();

  Serial.printf("------------------------\n"
                "Mesh ST:%s\n"
                "Node ID: %u\n"
                "Mesh clients: %d\n"
                "________________________\n"
                "Status WIFI Information:\n"
                "------------------------\n"
                "Local clients connected: %d\n"
                "Wifi connected: %s\n",
                thisNodeId ? "Enabled" : "Disabled",
                thisNodeId,
                meshClients,
                localClients,
                wifiConnected ? "true" : "false");

  if (wifiConnected) {
    Serial.printf(
      "------------------------\n"
      "STA Information:\n"
      "SSID: %s\n"
      "IP address: %s\n"
      "RSSI: %d dBm\n",
      ssid.c_str(),
      IP.toString().c_str(),
      rssi);
  }
  if (apClients) {
    Serial.printf("------------------------\n"
                  "AP clients: %d\n"
                  "AP Information:\n"
                  "AP IP address: %s\n"
                  "AP Tx Power: %d dBm\n"
                  "________________________\n\n",
                  apClients,
                  apIP.toString().c_str(),
                  apTxPower);
  }

  // Вывод информации о нодах
  Serial.println("\nCurrent Node List:");
  SimpleList<uint32_t>::iterator nodeIter = mesh.getNodeList().begin();
  while (nodeIter != mesh.getNodeList().end()) {
    String nodeType = (*nodeIter & 1) ? "AP" : "STA";                      // Определяем тип узла
    Serial.printf("Node ID: %u Type: %s\n", *nodeIter, nodeType.c_str());  // Выводим информацию о типе узла
    ++nodeIter;
  }

  Serial.printf("Free heap memory: %d bytes\n", ESP.getFreeHeap());
}