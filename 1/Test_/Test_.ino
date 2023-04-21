/*
Используем библиотеку PainlessMesh 1.5.0:
проверяй функции на совместимость перед тем, как использовать.
*/
#include <painlessMesh.h>
#include <ArduinoJson.h>
#include <vector>

#define MESH_SSID "your_mesh_ssid"
#define MESH_PASSWORD "your_mesh_password"
#define MESH_PORT 5555

painlessMesh mesh;
Scheduler userScheduler;
std::vector<Task *> addedTasks;
std::vector<Task *> tasksList;

void removeAllTasks() {
  for (auto &task : addedTasks) {
    task->disable();
  }
}


uint32_t thisNodeId;
uint32_t bestNodeId;

struct SystemInfo {
  uint32_t nodeId, apClients, localClients, apTxPower, rssi, meshClients, oldApClients, oldLocalClients, oldApTxPower, oldRssi, oldMeshClients;
  String ssid, subConnectionsJson, oldSsid, oldSubConnectionsJson;
  IPAddress apIP, IP, oldApIP, oldIP;
  bool wifiConnected, oldWifiConnected, meshConnected, oldMeshConnected, meshEnabled;
  time_t updateTime, oldUpdateTime;
};

SystemInfo systemInfo;

std::map<uint32_t, std::vector<std::pair<uint32_t, String>>> connectionsTable;
std::map<uint32_t, std::vector<std::pair<uint32_t, String>>> previousConnectionsTable;

void PrintWifiStatus();
void PrintChengeWifiStatus(SystemInfo &sysInfo);

Task printConnectionsTableTask(30000, TASK_FOREVER, []() {
  static bool taskAdded = false;
  if (!taskAdded) {
    taskAdded = true;
  }
  printConnectionsTableTask.enableDelayed(30000);  // повторяем через 30 секунд после выполнения
  Serial.printf("Запущено задание вывода таблицы соединений\n");
  PrintWifiStatus();
});

void disableMesh() {
  if (systemInfo.meshEnabled) {
    systemInfo.meshEnabled = false;
    mesh.stop();
    Serial.println("Mesh disabled");
  } else Serial.println("Alredy disabled Mesh");
}


Task disableMeshTask(6000, TASK_ONCE, []() {
  disableMesh();
  //removeAllTasks();
});




void onChangedConnectionsCallback() {
  systemInfo.meshConnected = thisNodeId;
  Serial.printf("* Mesh Connected: %s\n", systemInfo.meshConnected ? "true" : "false");
  PrintChengeWifiStatus(systemInfo);
  PrintWifiStatus();
}



void setup() {
  systemInfo.meshEnabled = true;
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE | APPLICATION | STARTUP);
  if (systemInfo.meshEnabled) mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onChangedConnections(&onChangedConnectionsCallback);
  //mesh.onReceive(&receivedCallback);
  userScheduler.addTask(printConnectionsTableTask);
  userScheduler.addTask(disableMeshTask);
  tasksList.push_back(&printConnectionsTableTask);
  tasksList.push_back(&disableMeshTask);
  printConnectionsTableTask.enableDelayed(30000);
  thisNodeId = mesh.getNodeId();
}

void loop() {
  if (systemInfo.meshEnabled) mesh.update();
  userScheduler.execute();
  processSerialCommand();
}

void PrintChengeWifiStatus(SystemInfo &sysInfo) {
  bool hasChanges = false;

  // Check for changes
  if (sysInfo.oldMeshClients != sysInfo.meshClients) {
    hasChanges = true;
    Serial.printf("Warning: Mesh clients changed. New value: %d*\n", sysInfo.meshClients);
  }
  if (sysInfo.oldSsid != sysInfo.ssid) {
    hasChanges = true;
    Serial.printf("Warning: SSID changed. New value: %s*\n", sysInfo.ssid.c_str());
  }
  if (sysInfo.oldApIP != sysInfo.apIP) {
    hasChanges = true;
    Serial.printf("Warning: AP IP changed. New value: %s*\n", sysInfo.apIP.toString().c_str());
  }
  if (sysInfo.oldIP != sysInfo.IP) {
    hasChanges = true;
    Serial.printf("Warning: Local IP changed. New value: %s*\n", sysInfo.IP.toString().c_str());
  }
  if (sysInfo.oldWifiConnected != sysInfo.wifiConnected) {
    hasChanges = true;
    Serial.printf("Warning: Wifi connection changed. New value: %s*\n", sysInfo.wifiConnected ? "true" : "false");
  }
  if (sysInfo.oldApClients != sysInfo.apClients) {
    hasChanges = true;
    Serial.printf("Warning: AP clients changed. New value: %d*\n", sysInfo.apClients);
  }
  if (sysInfo.oldApTxPower != sysInfo.apTxPower) {
    hasChanges = true;
    Serial.printf("Warning: AP Tx Power changed. New value: %d dBm*\n", sysInfo.apTxPower);
  }
  if (sysInfo.oldRssi != sysInfo.rssi) {
    hasChanges = true;
    Serial.printf("Warning: Wifi RSSI changed. New value: %d dBm*\n", sysInfo.rssi);
  }
  if (sysInfo.oldSubConnectionsJson != sysInfo.subConnectionsJson) {
    hasChanges = true;
    Serial.printf("Warning: Subconnections JSON changed. New value: %s*\n", sysInfo.subConnectionsJson.c_str());
  }
  if (sysInfo.oldMeshConnected != sysInfo.meshConnected) {
    hasChanges = true;
    Serial.printf("Warning: Mesh connection changed. New value: %s*\n", sysInfo.meshConnected ? "true" : "false");
  }

  if (hasChanges) {
    sysInfo.oldMeshClients = sysInfo.meshClients;
    sysInfo.oldSsid = sysInfo.ssid;
    sysInfo.oldApIP = sysInfo.apIP;
    sysInfo.oldIP = sysInfo.IP;
    sysInfo.oldWifiConnected = sysInfo.wifiConnected;
    sysInfo.oldApClients = sysInfo.apClients;
    sysInfo.oldApTxPower = sysInfo.apTxPower;
    sysInfo.oldRssi = sysInfo.rssi;
    sysInfo.oldSubConnectionsJson = sysInfo.subConnectionsJson;
    sysInfo.oldMeshConnected = sysInfo.meshConnected;
    sysInfo.oldUpdateTime = sysInfo.updateTime;

    Serial.printf("Changes detected at %s\n", ctime(&sysInfo.updateTime));
  }
  if (hasChanges) {
    PrintWifiStatus();
  }
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
  thisNodeId = mesh.getNodeId();

  Serial.printf("------------------------\n"
                "Mesh ST:%s\n"
                "Node ID: %u\n"
                "Mesh clients: %d\n"
                "________________________\n"
                "Status WIFI Information:\n"
                "------------------------\n"
                "Local clients connected: %d\n"
                "Wifi connected: %s\n",
                systemInfo.meshEnabled ? "Enabled" : "Disabled",
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
  printTasks();
}

void printTasks() {
  Serial.printf("Total %d task(s) found:\n", tasksList.size());
  int taskIndex = 0;
  for (auto &task : tasksList) {
    Serial.printf("Task Index: %d, enabled: %s, interval: %lu, iterations left: %ld\n",
                  taskIndex, task->isEnabled() ? "yes" : "no", task->getInterval(), task->getIterations());
    taskIndex++;
  }
}


void processSerialCommand() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "eM") {
      enableMesh();
    } else if (cmd == "dM") {
      Serial.println("Start <<Mesh>> off");
      disableMeshTask.restartDelayed(4000);
    } else if (cmd == "Z") {
      PrintWifiStatus();
    }
  }
}

void enableMesh() {
  if (!systemInfo.meshEnabled) {
    systemInfo.meshEnabled = true;
    mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
    Serial.println("<<<<  Mesh ON   >>> ");
  } else Serial.println("Mesh Alredy enabled");
}




void updateConnectionsTable() {
  
}