module_process_info
===================
  разработать модуль ядра и консольное приложение

  консольное приложение в момент запуска через аргументы командной строки получает один или более идентификаторов процессов

  передаёт каждый из них в модуль ядра и от него получает для каждого идентификатора следующую информацию..
  =========================================================================================================
    1 | Идентификатор пользователя, от имени которого запущен процесс
    2 | Путь к исполняемому файлу процесса
    3 | Командная строка запуска

  консольное приложение распечатываем полученную информацию и завершает работу

  консольное приложение должно быть реализовано на языке C, компилятор: gcc, система сборки: cmake
  ОС: Ubuntu. Ядро ОС: >= 5.15

  способ взаимодействия между приложением и модулем ядра — любой
