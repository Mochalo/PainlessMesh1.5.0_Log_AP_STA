#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <list>



#define MESH_PREFIX "your_mesh_ssid"
#define MESH_PASSWORD "your_mesh_password"
#define MESH_PORT 5555

Scheduler userScheduler;
painlessMesh mesh;



// Глобальная переменная для хранения информации о типе узлов
std::map<uint32_t, uint8_t> nodeTypes;
void removeInactiveNodes() {
  std::list<uint32_t> activeNodes = mesh.getNodeList();
  std::vector<uint32_t> nodesToRemove;

  // Находим узлы для удаления
  for (const auto &nodeTypeEntry : nodeTypes) {
    uint32_t nodeId = nodeTypeEntry.first;
    if (std::find(activeNodes.begin(), activeNodes.end(), nodeId) == activeNodes.end()) {
      nodesToRemove.push_back(nodeId);
    }
  }
  
  // Удаляем неактивные узлы
  for (uint32_t nodeId : nodesToRemove) {
    nodeTypes.erase(nodeId);
    Serial.printf("%s: Removing inactive node: %u\n", getCurrentTime().c_str(), nodeId);
  }
}

void processLogLine(const String &logLine) {
  if (logLine.startsWith("log: New AP Connection")) {
    int nodeIdIndex = logLine.indexOf("nodeId = ") + 8;
    int nodeIdEndIndex = logLine.indexOf(",", nodeIdIndex);
    uint32_t nodeId = logLine.substring(nodeIdIndex, nodeIdEndIndex).toInt();
    nodeTypes[nodeId] = 1; // AP
    Serial.printf("%s: Detected new AP connection from node %u\n", getCurrentTime().c_str(), nodeId);
  } else if (logLine.startsWith("log: New STA Connection")) {
    int nodeIdIndex = logLine.indexOf("nodeId = ") + 8;
    int nodeIdEndIndex = logLine.indexOf(",", nodeIdIndex);
    uint32_t nodeId = logLine.substring(nodeIdIndex, nodeIdEndIndex).toInt();
    nodeTypes[nodeId] = 0; // STA
    Serial.printf("%s: Detected new STA connection from node %u\n", getCurrentTime().c_str(), nodeId);
  }
}

void receivedCallback(uint32_t from, String &msg) {
  String logLine = String("log: Received from ") + String(from) + String(" msg=") + msg;
  processLogLine(logLine);
  Serial.printf("%s: %s\n", getCurrentTime().c_str(), logLine.c_str());
}

void newConnectionCallback(uint32_t nodeId, bool isAP) {
  uint32_t current_node_id = mesh.getNodeId();
  String logLine = String("log: New ") + (isAP ? "AP" : "STA") + String(" Connection, nodeId = ") + String(nodeId) + String(", connected to ") + String(current_node_id);
  processLogLine(logLine);
  Serial.printf("%s: %s\n", getCurrentTime().c_str(), logLine.c_str());
  // Здесь уже не нужно обновлять nodeTypes, так как мы делаем это в processLogLine()
}

void printstatusMesh() {
  printAllLogs();
  PrintWifiStatus();
  printMeshJSON();
}

String getCurrentTime() {
  time_t now;
  time(&now);
  char buf[sizeof "YYYY-MM-DD HH:MM:SS"];
  strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", localtime(&now));
  return String(buf);
}
  // Удаляем неакт
Task printTableTask(30000, TASK_FOREVER, printstatusMesh);

void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | APPLICATION | STARTUP);
  //mesh.setDebugMsgTypes(ERROR | MESH_STATUS | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection([&](uint32_t nodeId) {
    newConnectionCallback(nodeId, true);
  });
  mesh.onChangedConnections([]() {
    removeInactiveNodes();  // Удалить неактивные узлы при изменении списка узлов
  });
  mesh.onNodeTimeAdjusted([](int32_t offset) {
    // Выполнить дополнительные действия при синхронизации времени, если необходимо
  });



  userScheduler.addTask(printTableTask);
  printTableTask.enable();
}

void loop() {
  mesh.update();
}


void printAllLogs() {
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


  Serial.printf("Free heap memory: %d bytes\n", ESP.getFreeHeap());
}





void printMeshJSON() {
  Serial.println(mesh.subConnectionJson());
 aprintstatusMesh() ;
}


void aprintstatusMesh() {
  Serial.println("Node types:");
  for (auto const& nodeType : nodeTypes) {
    Serial.printf("%u - %s\n", nodeType.first, nodeType.second == 0 ? "STA" : "AP");
  }
  Serial.println();

  String meshJson = mesh.subConnectionJson();
  Serial.println(meshJson);
}







