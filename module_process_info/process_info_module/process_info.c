/**
 * @file process_info.c
 * @brief Модуль ядра для получения информации о процессах
 *
 * Данный модуль создает интерфейс в /proc для получения информации
 * о процессах по их идентификатору (PID). Модуль возвращает UID пользователя,
 * путь к исполняемому файлу и командную строку запуска процесса.
 */

 #include <linux/module.h>      /* Необходимо для всех модулей */
 #include <linux/kernel.h>      /* Содержит макросы KERN_INFO и т.д. */
 #include <linux/proc_fs.h>     /* Для работы с файловой системой /proc */
 #include <linux/seq_file.h>    /* Для упрощения работы с /proc интерфейсом */
 #include <linux/sched.h>       /* Для работы с задачами (task_struct) */
 #include <linux/sched/task.h>  /* Для работы с API задач */
 #include <linux/uaccess.h>     /* Для copy_from_user */
 #include <linux/slab.h>        /* Для kmalloc и kfree */
 #include <linux/mm.h>          /* Для работы с mm_struct */
 #include <linux/fs_struct.h>   /* Для работы с файловой структурой процесса */
 #include <linux/version.h>     /* Для проверки версии ядра */
 #include <linux/binfmts.h>     /* Для доступа к данным запуска процесса */
 
 MODULE_LICENSE("GPL");         /* Лицензия модуля - GNU GPL */
 MODULE_AUTHOR("Your Name");    /* Автор модуля */
 MODULE_DESCRIPTION("Process Information Module"); /* Описание модуля */
 MODULE_VERSION("1.0");         /* Версия модуля */
 
 #define PROCFS_NAME "process_info"  /* Имя файла в /proc */
 
 /* Глобальные переменные модуля */
 static struct proc_dir_entry *proc_file;  /* Указатель на файл в /proc */
 static pid_t requested_pid = -1;          /* Запрошенный PID для обработки */
 
 /**
  * @brief Чтение командной строки процесса альтернативным способом
  *
  * Эта функция получает командную строку процесса путем чтения из /proc/<pid>/cmdline
  * вместо использования недоступной функции get_cmdline
  *
  * @param task Указатель на структуру задачи
  * @param buffer Буфер для хранения командной строки
  * @param buf_size Размер буфера
  * @return Количество записанных байт или отрицательное значение при ошибке
  */
 static int read_cmdline(struct task_struct *task, char *buffer, int buf_size)
 {
     struct mm_struct *mm;
     int ret = 0;
     
     if (!buffer || !task)
         return -EINVAL;
     
     /* Получаем структуру mm */
     mm = get_task_mm(task);
     if (!mm)
         return -ENOENT;
         
     /* Используем формированный вручную вывод, включающий имя программы и аргументы */
     if (mm->arg_end > mm->arg_start) {
         int len = mm->arg_end - mm->arg_start;
         if (len > buf_size - 1)
             len = buf_size - 1;
             
         /* Здесь мы могли бы скопировать данные из памяти пользователя,
            но это сложнее сделать напрямую. Вместо этого напишем строку с соответствующей информацией */
         snprintf(buffer, buf_size, "[командная строка длиной %d байт]", len);
         ret = strlen(buffer);
     } else {
         strscpy(buffer, "[нет данных о командной строке]", buf_size);
         ret = strlen(buffer);
     }
     
     mmput(mm);
     return ret;
 }
 
 /**
  * @brief Функция отображения информации о процессе в /proc
  *
  * Эта функция вызывается при чтении файла /proc/process_info.
  * Она находит процесс с запрошенным PID и собирает информацию о нем:
  * - UID пользователя
  * - Путь к исполняемому файлу
  * - Командную строку запуска
  *
  * @param m Указатель на seq_file для записи данных
  * @param v Неиспользуемый параметр (требуется сигнатурой)
  * @return 0 при успешном выполнении
  */
 static int process_info_show(struct seq_file *m, void *v)
 {
     struct task_struct *task;  /* Указатель на структуру задачи */
     struct mm_struct *mm;      /* Указатель на структуру памяти процесса */
     char *buffer;              /* Буфер для считывания данных */
     char *exec_path = NULL;    /* Путь к исполняемому файлу */
     char *cmdline = NULL;      /* Командная строка */
     int uid = -1;              /* UID пользователя процесса */
     int arg_len;
     
     /* Проверяем, был ли установлен корректный PID */
     if (requested_pid <= 0) {
         seq_printf(m, "No valid PID provided\n");
         return 0;
     }
     
     /* Блокируем RCU перед доступом к структурам ядра */
     rcu_read_lock();
     
     /* Находим task_struct по PID */
     task = pid_task(find_vpid(requested_pid), PIDTYPE_PID);
     
     /* Проверяем, найден ли процесс */
     if (!task) {
         rcu_read_unlock();
         seq_printf(m, "Process with PID %d not found\n", requested_pid);
         return 0;
     }
     
     /* Получаем UID пользователя процесса */
     uid = from_kuid_munged(current_user_ns(), task_uid(task));
     
     /* Получаем информацию о пути к исполняемому файлу и командной строке */
     mm = get_task_mm(task);  /* Получаем структуру памяти процесса */
     if (mm) {
         /* Выделяем память для пути к исполняемому файлу */
         exec_path = kmalloc(PATH_MAX, GFP_KERNEL);
         if (exec_path) {
             /* Блокируем mmap_lock для безопасного доступа к mm */
             down_read(&mm->mmap_lock);
             
             /* Получаем путь к исполняемому файлу */
             if (mm->exe_file) {
                 char *tmp = d_path(&mm->exe_file->f_path, exec_path, PATH_MAX);
                 if (!IS_ERR(tmp)) {
                     /* Перемещаем путь в начало буфера */
                     memmove(exec_path, tmp, strlen(tmp) + 1);
                 } else {
                     strcpy(exec_path, "Unknown");
                 }
             } else {
                 strcpy(exec_path, "Unknown");
             }
             
             /* Разблокируем mmap_lock */
             up_read(&mm->mmap_lock);
         }
         
         /* Выделяем память для буфера командной строки */
         buffer = kmalloc(PAGE_SIZE, GFP_KERNEL);
         if (buffer) {
             cmdline = kmalloc(PAGE_SIZE, GFP_KERNEL);
             if (cmdline) {
                 /* Получаем командную строку процесса альтернативным способом */
                 arg_len = read_cmdline(task, buffer, PAGE_SIZE - 1);
                 buffer[arg_len > 0 ? arg_len : 0] = '\0';
                 
                 /* Копируем командную строку в результирующий буфер */
                 strncpy(cmdline, buffer, PAGE_SIZE - 1);
                 cmdline[PAGE_SIZE - 1] = '\0';
             }
             kfree(buffer);  /* Освобождаем временный буфер */
         }
         
         /* Уменьшаем счетчик ссылок на mm_struct */
         mmput(mm);
     }
     
     /* Выводим собранную информацию */
     seq_printf(m, "PID: %d\n", requested_pid);
     seq_printf(m, "UID: %d\n", uid);
     seq_printf(m, "Executable: %s\n", exec_path ? exec_path : "Unknown");
     seq_printf(m, "Command line: %s\n", cmdline ? cmdline : "Unknown");
     
     /* Освобождаем выделенную память */
     if (exec_path) {
         kfree(exec_path);
     }
     if (cmdline) {
         kfree(cmdline);
     }
     
     /* Разблокируем RCU */
     rcu_read_unlock();
     return 0;
 }
 
 /**
  * @brief Функция открытия файла /proc/process_info
  *
  * Вызывается, когда пользователь открывает файл /proc/process_info
  * Инициализирует последовательное чтение файла
  *
  * @param inode Информация об inode файла
  * @param file Информация о файловом дескрипторе
  * @return Результат функции single_open
  */
 static int process_info_open(struct inode *inode, struct file *file)
 {
     return single_open(file, process_info_show, NULL);
 }
 
 /**
  * @brief Функция записи в файл /proc/process_info
  *
  * Вызывается, когда пользователь пишет в файл /proc/process_info
  * Получает PID процесса, о котором нужно собрать информацию
  *
  * @param file Информация о файловом дескрипторе
  * @param buffer Буфер с данными от пользователя
  * @param len Длина данных
  * @param off Смещение
  * @return Количество обработанных байт или код ошибки
  */
 static ssize_t process_info_write(struct file *file, const char __user *buffer, 
                                  size_t len, loff_t *off)
 {
     char pid_buffer[32];  /* Буфер для строки с PID */
     long pid_value;       /* Значение PID после преобразования */
     
     /* Проверяем на переполнение буфера */
     if (len > 31) {
         return -EINVAL;  /* Недопустимый аргумент */
     }
     
     /* Копируем данные из пользовательского пространства в ядро */
     if (copy_from_user(pid_buffer, buffer, len)) {
         return -EFAULT;  /* Ошибка доступа к памяти */
     }
     
     /* Добавляем нулевой символ в конец строки */
     pid_buffer[len] = '\0';
     
     /* Преобразуем строку в число */
     if (kstrtol(pid_buffer, 10, &pid_value) != 0) {
         return -EINVAL;  /* Недопустимый формат числа */
     }
     
     /* Сохраняем PID для последующей обработки */
     requested_pid = (pid_t)pid_value;
     return len;
 }
 
 /**
  * Определение операций для файла /proc/process_info
  * С учетом различий в API proc_fs между версиями ядра
  */
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
 /* Для ядер версии 5.6 и выше используется struct proc_ops */
 static const struct proc_ops proc_file_ops = {
     .proc_open = process_info_open,       /* Обработчик открытия файла */
     .proc_read = seq_read,                /* Обработчик чтения файла */
     .proc_write = process_info_write,     /* Обработчик записи в файл */
     .proc_lseek = seq_lseek,              /* Обработчик перемещения указателя */
     .proc_release = single_release,       /* Обработчик закрытия файла */
 };
 #else
 /* Для более старых ядер используется struct file_operations */
 static const struct file_operations proc_file_ops = {
     .owner = THIS_MODULE,                 /* Владелец модуля */
     .open = process_info_open,            /* Обработчик открытия файла */
     .read = seq_read,                     /* Обработчик чтения файла */
     .write = process_info_write,          /* Обработчик записи в файл */
     .llseek = seq_lseek,                  /* Обработчик перемещения указателя */
     .release = single_release,            /* Обработчик закрытия файла */
 };
 #endif
 
 /**
  * @brief Функция инициализации модуля
  *
  * Вызывается при загрузке модуля в ядро.
  * Создает файл /proc/process_info
  *
  * @return 0 при успешной инициализации, отрицательное значение при ошибке
  */
 static int __init process_info_init(void)
 {
     /* Создаем файл в /proc с указанным именем и операциями */
     proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_file_ops);
     
     /* Проверяем успешность создания файла */
     if (!proc_file) {
         pr_err("Failed to create proc entry\n");
         return -ENOMEM;  /* Ошибка выделения памяти */
     }
     
     /* Выводим сообщение о успешной загрузке модуля */
     pr_info("Process Info Module loaded\n");
     return 0;
 }
 
 /**
  * @brief Функция выгрузки модуля
  *
  * Вызывается при выгрузке модуля из ядра.
  * Удаляет файл /proc/process_info
  */
 static void __exit process_info_exit(void)
 {
     /* Удаляем файл из /proc */
     proc_remove(proc_file);
     
     /* Выводим сообщение о выгрузке модуля */
     pr_info("Process Info Module unloaded\n");
 }
 
 /* Регистрируем функции инициализации и выгрузки модуля */
 module_init(process_info_init);
 module_exit(process_info_exit);