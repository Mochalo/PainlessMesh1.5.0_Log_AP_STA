Используем библиотеку PainlessMesh 1.5.0 на Arduino Ide Разбивая на функции с переносам на другие листына ё листе просто названия функций чтоб выполнения на втором.
Вместо Serial.print, Serial.println используем Serial.printf (группируем printf)
Вместо delay, таймеров на if (millis() используем Scheduler userScheduler
На основе лога выделяем нужные значения, создаем временный массив с нужными данными (анализируем log.txt)
Цель задачи: определить типы подключенных между собой нод AP,STA, создать массив и при отсутствии задачи на 1 секунду вперед записывать лог на внутреннюю память.
Создаём структуру  для хранения данных 

Пример кода с использованием userScheduler:
WIFI Есть в библиотеке  painlessMesh.h

Это не код это приме Scheduler {
#include <painlessMesh.h>

painlessMesh mesh;
Scheduler userScheduler;

void run();
Task myTask(1000, TASK_FOREVER, []() { run(); });

void run() { 
  Serial.printf("Hello from task!");
}

void setup() {
  mesh.init("MyMesh");
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION);
  userScheduler.addTask(myTask);
  myTask.enable();
}

void loop() {
  mesh.update();
  userScheduler.execute();
}
}
Весь код находится по ссылке: https://github.com/Mochalo/PainlessMesh1.5.0
Программа Test_.ino
log  Log.txt
Инструкция FirstRead.txt
Код мне выводить разбивай по функциям и нумируя
если я ковори посмотри за по ссылке https://github.com/Mochalo/PainlessMesh1.5.0_Log_AP_STA