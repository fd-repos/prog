#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import shutil
import re

def parse_arguments():
    """Функция для разбора аргументов командной строки."""
    # Проверяем корректное количество аргументов
    if len(sys.argv) != 3:
        print("Использование: python cp_folder.py src=<исходный_путь> dst=<путь_назначения>")
        sys.exit(1)
    
    # Парсим аргументы src и dst
    src_path = None
    dst_path = None
    
    for arg in sys.argv[1:]:
        if arg.startswith("src="):
            src_path = arg[4:]
        elif arg.startswith("dst="):
            dst_path = arg[4:]
    
    # Раскрываем путь пользователя (например, ~/)
    if src_path:
        src_path = os.path.expanduser(src_path)
    if dst_path:
        dst_path = os.path.expanduser(dst_path)
    
    # Нормализуем пути
    if src_path:
        src_path = os.path.normpath(src_path)
    if dst_path:
        dst_path = os.path.normpath(dst_path)
    
    if not src_path or not dst_path:
        print("Ошибка: Должны быть указаны оба пути src и dst.")
        sys.exit(1)
    
    return src_path, dst_path

def print_step_result(step_number, description, result):
    """Функция для форматированного вывода результатов шагов."""
    # Форматируем вывод с выравниванием символов |
    result_str = "ok" if result else "bad"
    
    # Убеждаемся, что описание имеет фиксированную ширину для выравнивания
    padded_description = description.ljust(35)
    
    print(f"{step_number} | {padded_description} | {result_str}")
    
    # Выходим, если результат отрицательный
    if not result:
        sys.exit(1)

def get_folder_name(path):
    """Извлечь имя папки из пути."""
    return os.path.basename(path)

def check_folder_names(src_path, dst_path):
    """Проверить совпадение имен исходной и целевой папок.
    
    Это шаг 1 логики копирования папок:
    1. Проверить, совпадают ли имена папок src и dst
    """
    src_name = get_folder_name(src_path)
    dst_name = get_folder_name(dst_path)
    
    return src_name == dst_name

def clean_trash(src_path, dst_path):
    """Очистить мусор и ненужные файлы из папки dst.
    
    Это шаг 2 логики копирования папок:
    2. Удалить мусор из папок
    
    Примечание: src не должен быть изменен согласно требованиям.
    """
    try:
        # Определяем, что считается "мусором":
        # - Файлы/папки с именами "trash" или ".trash"
        # - Временные файлы (например, заканчивающиеся на ~, .tmp, .temp)
        # - Скрытые системные файлы (например .DS_Store на Mac или Thumbs.db в Windows)
        trash_patterns = [
            r"^trash$", r"^\.trash$",  # trash и .trash файлы/папки
            r".*~$", r".*\.tmp$", r".*\.temp$",  # временные файлы
            r"^\.DS_Store$", r"^Thumbs\.db$"  # системные файлы
        ]
        
        # Не модифицируем src согласно требованиям
        # Очищаем мусор только в dst, если он существует
        if os.path.exists(dst_path):
            for root, dirs, files in os.walk(dst_path, topdown=False):
                # Сначала обрабатываем файлы
                for file_name in files:
                    if any(re.match(pattern, file_name, re.IGNORECASE) for pattern in trash_patterns):
                        file_path = os.path.join(root, file_name)
                        os.remove(file_path)
                
                # Затем обрабатываем директории
                for dir_name in dirs:
                    if any(re.match(pattern, dir_name, re.IGNORECASE) for pattern in trash_patterns):
                        dir_path = os.path.join(root, dir_name)
                        shutil.rmtree(dir_path)
        
        return True
    except Exception as e:
        print(f"Ошибка при очистке мусора: {e}")
        return False

def sync_folder_structure(src_path, dst_path):
    """Синхронизировать структуру папок между src и dst.
    
    Это шаг 3 логики копирования папок:
    3. Сопоставить деревья папок src и dst путём создания недостающих папок
       в dst и удаления тех, которых нет в src
    """
    try:
        # Создаем dst, если он не существует
        if not os.path.exists(dst_path):
            os.makedirs(dst_path)
        
        # Получаем все директории в src с их относительными путями
        src_dirs = set()
        for root, dirs, _ in os.walk(src_path):
            for dir_name in dirs:
                # Получаем относительный путь от src
                full_dir_path = os.path.join(root, dir_name)
                relative_path = os.path.relpath(full_dir_path, src_path)
                src_dirs.add(relative_path)
        
        # Получаем все директории и файлы в dst с их относительными путями
        dst_dirs = set()
        dst_files = set()
        if os.path.exists(dst_path):
            for root, dirs, files in os.walk(dst_path):
                for dir_name in dirs:
                    # Получаем относительный путь от dst
                    full_dir_path = os.path.join(root, dir_name)
                    relative_path = os.path.relpath(full_dir_path, dst_path)
                    dst_dirs.add(relative_path)
                
                for file_name in files:
                    # Получаем относительный путь от dst для файлов
                    full_file_path = os.path.join(root, file_name)
                    relative_path = os.path.relpath(full_file_path, dst_path)
                    dst_files.add(relative_path)
        
        # Проверяем и устраняем конфликты: если файл в dst имеет такой же путь, 
        # как директория в src, удаляем файл
        for dir_path in src_dirs:
            if dir_path in dst_files:
                conflicting_file = os.path.join(dst_path, dir_path)
                os.remove(conflicting_file)
                print(f"Удален конфликтующий файл: {conflicting_file}")
        
        # Создаем отсутствующие директории в dst с безопасным параметром exist_ok
        for dir_path in src_dirs:
            full_dst_path = os.path.join(dst_path, dir_path)
            # Используем exist_ok=True для предотвращения ошибок при параллельном создании директорий
            try:
                os.makedirs(full_dst_path, exist_ok=True)
            except FileExistsError:
                # Если путь существует, но это не директория, преобразуем его
                if os.path.exists(full_dst_path) and not os.path.isdir(full_dst_path):
                    os.remove(full_dst_path)
                    os.makedirs(full_dst_path)
        
        # Удаляем директории в dst, которых нет в src
        for dir_path in dst_dirs:
            if dir_path not in src_dirs:
                full_dst_path = os.path.join(dst_path, dir_path)
                try:
                    # Проверяем существование директории перед удалением
                    if os.path.exists(full_dst_path) and os.path.isdir(full_dst_path):
                        shutil.rmtree(full_dst_path)
                    # Если директория была в списке, но уже не существует, пропускаем
                except PermissionError:
                    print(f"Невозможно удалить директорию из-за ограничений доступа: {full_dst_path}")
                except FileNotFoundError:
                    # Директория могла быть удалена между составлением списка и удалением
                    pass
                except Exception as e:
                    print(f"Ошибка при удалении директории {full_dst_path}: {e}")
        
        return True
    except Exception as e:
        print(f"Ошибка при синхронизации структуры папок: {e}")
        return False

def sync_files(src_path, dst_path):
    """Синхронизировать файлы между src и dst.
    
    Это шаг 4 логики копирования папок:
    4. Сопоставить файлы между src и dst:
       - Удалить файлы в dst, которых нет в src
       - Скопировать файлы из src в dst, которых нет в dst
       - Обновить файлы в dst, у которых более старая метка времени, чем в src
       - Пропустить файлы с одинаковой меткой времени
    """
    try:
        # Получаем все файлы в src с их относительными путями и временем модификации
        src_files = {}
        src_symlinks = {}
        
        # Обрабатываем обычные файлы и символические ссылки отдельно
        for root, _, files in os.walk(src_path):
            for file_name in files:
                # Получаем полный и относительный пути
                full_path = os.path.join(root, file_name)
                relative_path = os.path.relpath(full_path, src_path)
                
                # Проверяем, является ли файл символической ссылкой
                if os.path.islink(full_path):
                    # Сохраняем информацию о символической ссылке
                    link_target = os.readlink(full_path)
                    src_symlinks[relative_path] = link_target
                else:
                    # Получаем время модификации для обычных файлов
                    try:
                        mod_time = os.path.getmtime(full_path)
                        src_files[relative_path] = mod_time
                    except OSError as e:
                        print(f"Предупреждение: Не удалось получить время модификации для {full_path}: {e}")
        
        # Получаем все файлы в dst с их относительными путями и временем модификации
        dst_files = {}
        dst_symlinks = {}
        
        if os.path.exists(dst_path):
            for root, _, files in os.walk(dst_path):
                for file_name in files:
                    # Получаем полный и относительный пути
                    full_path = os.path.join(root, file_name)
                    relative_path = os.path.relpath(full_path, dst_path)
                    
                    # Проверяем, является ли файл символической ссылкой
                    if os.path.islink(full_path):
                        # Сохраняем информацию о символической ссылке
                        link_target = os.readlink(full_path)
                        dst_symlinks[relative_path] = link_target
                    else:
                        # Получаем время модификации для обычных файлов
                        try:
                            mod_time = os.path.getmtime(full_path)
                            dst_files[relative_path] = mod_time
                        except OSError as e:
                            print(f"Предупреждение: Не удалось получить время модификации для {full_path}: {e}")
        
        # Удаляем файлы и символические ссылки в dst, которых нет в src
        for file_path in list(dst_files.keys()):
            if file_path not in src_files and file_path not in src_symlinks:
                full_dst_path = os.path.join(dst_path, file_path)
                try:
                    os.remove(full_dst_path)
                except OSError as e:
                    print(f"Предупреждение: Не удалось удалить файл {full_dst_path}: {e}")
        
        for link_path in list(dst_symlinks.keys()):
            if link_path not in src_files and link_path not in src_symlinks:
                full_dst_path = os.path.join(dst_path, link_path)
                try:
                    os.remove(full_dst_path)
                except OSError as e:
                    print(f"Предупреждение: Не удалось удалить символическую ссылку {full_dst_path}: {e}")
        
        # Копируем или обновляем обычные файлы из src в dst
        for file_path, src_mod_time in src_files.items():
            full_src_path = os.path.join(src_path, file_path)
            full_dst_path = os.path.join(dst_path, file_path)
            
            # Проверяем, существует ли родительская директория
            parent_dir = os.path.dirname(full_dst_path)
            
            # Создаем все необходимые родительские директории
            try:
                if not os.path.exists(parent_dir):
                    os.makedirs(parent_dir, exist_ok=True)
            except OSError as e:
                print(f"Предупреждение: Не удалось создать директорию {parent_dir}: {e}")
                continue
            
            # Если файл не существует в dst или файл в src новее
            if (file_path not in dst_files or 
                src_mod_time > dst_files[file_path]):
                
                try:
                    # Копируем файл с его метаданными (временными метками и т.д.)
                    shutil.copy2(full_src_path, full_dst_path)
                except OSError as e:
                    print(f"Ошибка при копировании файла {full_src_path} в {full_dst_path}: {e}")
                    # Пытаемся использовать альтернативный метод копирования для файлов с особыми атрибутами
                    try:
                        with open(full_src_path, 'rb') as src_f:
                            with open(full_dst_path, 'wb') as dst_f:
                                dst_f.write(src_f.read())
                        # Копируем метаданные файла, если возможно
                        try:
                            shutil.copystat(full_src_path, full_dst_path)
                        except:
                            pass
                    except Exception as e:
                        print(f"Ошибка при альтернативном копировании файла {full_src_path}: {e}")
        
        # Обрабатываем символические ссылки
        for link_path, target in src_symlinks.items():
            full_dst_link = os.path.join(dst_path, link_path)
            
            # Проверяем, существует ли родительская директория
            parent_dir = os.path.dirname(full_dst_link)
            
            # Создаем все необходимые родительские директории
            try:
                if not os.path.exists(parent_dir):
                    os.makedirs(parent_dir, exist_ok=True)
            except OSError as e:
                print(f"Предупреждение: Не удалось создать директорию {parent_dir}: {e}")
                continue
            
            # Удаляем существующую ссылку или файл, если они есть
            if os.path.exists(full_dst_link) or os.path.islink(full_dst_link):
                try:
                    os.remove(full_dst_link)
                except OSError as e:
                    print(f"Предупреждение: Не удалось удалить существующую ссылку {full_dst_link}: {e}")
                    continue
            
            # Создаем символическую ссылку
            try:
                os.symlink(target, full_dst_link)
            except OSError as e:
                print(f"Ошибка при создании символической ссылки {full_dst_link} -> {target}: {e}")
        
        return True
    except Exception as e:
        print(f"Ошибка при синхронизации файлов: {e}")
        return False

def main():
    """Основная функция для выполнения логики копирования папок."""
    
    print("")
    
    # Разбор аргументов командной строки
    src_path, dst_path = parse_arguments()
    
    # Проверяем существование исходного пути
    if not os.path.exists(src_path):
        print(f"Ошибка: Исходный путь {src_path} не существует.")
        sys.exit(1)
    
    # Шаг 1: Проверяем, совпадают ли имена папок src и dst
    result = check_folder_names(src_path, dst_path)
    print_step_result(1, "проверить имена src == dst", result)
    
    # Шаг 2: Удаляем мусор
    result = clean_trash(src_path, dst_path)
    print_step_result(2, "удалить мусор", result)
    
    # Шаг 3: Синхронизируем структуру папок
    result = sync_folder_structure(src_path, dst_path)
    print_step_result(3, "сопоставить деревья папок src и dst", result)
    
    # Шаг 4: Синхронизируем файлы
    result = sync_files(src_path, dst_path)
    print_step_result(4, "сопоставить файлы из src и dst", result)
    
    print("")

if __name__ == "__main__":
    main()