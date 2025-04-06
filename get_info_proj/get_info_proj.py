#!/usr/bin/env python3
import os
import sys
import subprocess

def count_folders(directory):
    """Подсчет количества папок в директории и ее поддиректориях."""
    folder_count = 0
    for root, dirs, _ in os.walk(directory):
        folder_count += len(dirs)
    return folder_count

def count_files(directory):
    """Подсчет количества файлов в директории и ее поддиректориях."""
    file_count = 0
    for _, _, files in os.walk(directory):
        file_count += len(files)
    return file_count

def count_rows(directory):
    """Подсчет общего количества строк во всех файлах директории и ее поддиректорий."""
    row_count = 0
    for root, _, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            try:
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    row_count += sum(1 for _ in f)
            except:
                # Пропускаем файлы, которые нельзя прочитать как текст
                pass
    return row_count

def count_commits(directory):
    """Подсчет количества Git коммитов, если существует директория .git, иначе возвращает 'no'."""
    git_dir = os.path.join(directory, '.git')
    if not os.path.exists(git_dir):
        return "no"
    
    try:
        # Пробуем подсчитать коммиты с помощью git команды
        result = subprocess.run(['git', '-C', directory, 'rev-list', '--count', 'HEAD'], 
                                capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except:
        # Если возникла ошибка, предполагаем, что это не git репозиторий
        return "no"

def calculate_memory_usage(directory):
    """Расчет общего объема памяти, используемого директорией и ее содержимым в килобайтах."""
    total_size = 0
    for root, dirs, files in os.walk(directory):
        for file in files:
            file_path = os.path.join(root, file)
            try:
                total_size += os.path.getsize(file_path)
            except:
                # Пропускаем файлы, к которым нет доступа
                pass
    
    # Конвертируем в килобайты
    return f"{total_size // 1024} kB"

def main():
    # Разбор аргументов командной строки
    if len(sys.argv) != 2 or not sys.argv[1].startswith('proj='):
        print("Использование: python get_info_proj.py proj=path/to/project")
        return
    
    # Извлечение пути к директории проекта
    directory = sys.argv[1].split('=', 1)[1]
    
    # Проверка существования директории
    if not os.path.isdir(directory):
        print(f"Ошибка: {directory} не является допустимой директорией")
        return
    
    # Анализ проекта
    folder_count = count_folders(directory)
    file_count = count_files(directory)
    row_count = count_rows(directory)
    commit_count = count_commits(directory)
    memory_usage = calculate_memory_usage(directory)
    
    # Вывод результатов
    print(f"")
    print(f"1 | folder   | {folder_count}")
    print(f"2 | file     | {file_count}")
    print(f"3 | row      | {row_count}")
    print(f"4 | commit   | {commit_count}")
    print(f"5 | memory   | {memory_usage}")
    print(f"")
    print(f"version.{folder_count}.{file_count}.{row_count}.{commit_count}.{memory_usage}")
    print(f"")

if __name__ == "__main__":
    main()