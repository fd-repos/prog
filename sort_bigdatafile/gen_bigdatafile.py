import random
import string

def generate_key():
    # Генерируем случайное целое беззнаковое число формата unsigned long long (0 до 2^64-1)
    return random.randint(0, 2**64-1)

def generate_value():
    # Генерируем строку из 10 случайных цифро-буквенных символов
    length = random.randint(5, 15)
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(length))

def generate_file(num_lines):
    # Формируем имя файла
    filename = "gen_data.txt"
    
    # Создаем и записываем в файл указанное количество строк
    with open(filename, 'w') as file:
        for _ in range(num_lines):
            key = generate_key()
            value = generate_value()
            file.write(f"{key}:{value}\n")
    
    print(f"Файл {filename} успешно создан с {num_lines} строками.")

def main():
    try:
        # Запрашиваем у пользователя количество строк
        num_lines = int(input("Введите количество строк для генерации: "))
        
        # Проверяем, что число положительное
        if num_lines <= 0:
            print("Ошибка: Количество строк должно быть положительным числом.")
            return
        
        # Генерируем файл
        generate_file(num_lines)
        
    except ValueError:
        print("Ошибка: Введите корректное целое число.")
    except Exception as e:
        print(f"Произошла ошибка: {e}")

if __name__ == "__main__":
    main()