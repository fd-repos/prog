#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <cstdint>
#include <charconv>
#include <system_error>
#include <chrono>

// Структура для хранения пары ключ-значение и исходной позиции
struct KeyValuePair {
    uint64_t key;         // ключ
    std::string value;    // значение
    size_t originalIndex; // исходная позиция для обеспечения устойчивости сортировки
};

// Функция для разбора строки на ключ и значение
bool parseKeyValue(const std::string& line, KeyValuePair& pair) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false; // Не найдено двоеточие
    }

    // Попытка преобразовать ключ в uint64_t
    const char* start = line.data();
    const char* end = start + colonPos;
    auto result = std::from_chars(start, end, pair.key);
    if (result.ec != std::errc()) {
        return false; // Ошибка при преобразовании ключа
    }

    // Извлечение значения
    if (colonPos + 1 < line.size()) {
        pair.value = line.substr(colonPos + 1);
    } else {
        pair.value = ""; // Пустое значение
    }

    return true;
}

// Класс для пакетной обработки файла
class FileSorter {
private:
    const std::string inputPath;
    const std::string outputPath;
    
    // Максимальное количество строк для обработки за раз
    static constexpr size_t BATCH_SIZE = 1000000;
    
    // Создает временный файл и возвращает его имя
    std::string createTempFile(size_t index) const {
        return outputPath + ".temp" + std::to_string(index);
    }
    
    // Сортирует часть файла и записывает во временный файл
    bool sortBatch(std::ifstream& input, size_t batchIndex, size_t& linesProcessed) {
        std::vector<KeyValuePair> batch;
        batch.reserve(BATCH_SIZE);
        
        // Чтение строк из входного файла
        std::string line;
        size_t lineIndex = 0;
        while (batch.size() < BATCH_SIZE && std::getline(input, line)) {
            KeyValuePair pair;
            pair.originalIndex = linesProcessed + lineIndex;
            
            if (parseKeyValue(line, pair)) {
                batch.push_back(std::move(pair));
            } else {
                std::cerr << "Предупреждение: невозможно разобрать строку: " << line << std::endl;
            }
            lineIndex++;
        }
        
        if (batch.empty()) {
            return false; // Нет данных для обработки
        }
        
        // Устойчивая сортировка по ключу
        std::stable_sort(batch.begin(), batch.end(), 
                         [](const KeyValuePair& a, const KeyValuePair& b) {
                             return a.key < b.key;
                         });
        
        // Запись отсортированного пакета во временный файл
        const std::string tempFile = createTempFile(batchIndex);
        std::ofstream output(tempFile);
        if (!output) {
            std::cerr << "Ошибка: не удалось создать временный файл " << tempFile << std::endl;
            return false;
        }
        
        for (const auto& pair : batch) {
            output << pair.key << ":" << pair.value << "\n";
        }
        
        linesProcessed += lineIndex;
        return true;
    }
    
    // Объединяет все временные файлы в итоговый
    bool mergeTempFiles(size_t tempFileCount) {
        if (tempFileCount == 0) {
            return false;
        }
        
        // Если только один временный файл, просто переименовываем его
        if (tempFileCount == 1) {
            std::string tempFile = createTempFile(0);
            std::ifstream input(tempFile, std::ios::binary);
            std::ofstream output(outputPath, std::ios::binary);
            
            if (!input || !output) {
                std::cerr << "Ошибка при переименовании единственного временного файла" << std::endl;
                return false;
            }
            
            output << input.rdbuf();
            input.close();
            output.close();
            std::remove(tempFile.c_str());
            return true;
        }
        
        // Структура для многопутевого слияния
        struct FileEntry {
            std::ifstream file;
            KeyValuePair currentPair;
            bool hasNext;
            
            FileEntry(const std::string& path) : file(path), hasNext(false) {
                readNext();
            }
            
            void readNext() {
                std::string line;
                if (std::getline(file, line)) {
                    if (parseKeyValue(line, currentPair)) {
                        hasNext = true;
                    } else {
                        readNext(); // Пропускаем некорректные строки
                    }
                } else {
                    hasNext = false;
                }
            }
        };
        
        // Открываем все временные файлы
        std::vector<FileEntry> files;
        files.reserve(tempFileCount);
        
        for (size_t i = 0; i < tempFileCount; ++i) {
            files.emplace_back(createTempFile(i));
            if (!files.back().file) {
                std::cerr << "Ошибка: не удалось открыть временный файл " << createTempFile(i) << std::endl;
                return false;
            }
        }
        
        // Открываем выходной файл
        std::ofstream output(outputPath);
        if (!output) {
            std::cerr << "Ошибка: не удалось создать файл результата " << outputPath << std::endl;
            return false;
        }
        
        // Выполняем многопутевое слияние
        while (true) {
            // Находим файл с минимальным следующим ключом
            size_t minIndex = 0;
            bool foundValid = false;
            
            for (size_t i = 0; i < files.size(); ++i) {
                if (files[i].hasNext) {
                    if (!foundValid || 
                        (files[i].currentPair.key < files[minIndex].currentPair.key) ||
                        (files[i].currentPair.key == files[minIndex].currentPair.key && 
                         files[i].currentPair.originalIndex < files[minIndex].currentPair.originalIndex)) {
                        minIndex = i;
                        foundValid = true;
                    }
                }
            }
            
            if (!foundValid) {
                break; // Все файлы обработаны
            }
            
            // Записываем минимальную пару в выходной файл
            output << files[minIndex].currentPair.key << ":" << files[minIndex].currentPair.value << "\n";
            
            // Читаем следующую запись из этого файла
            files[minIndex].readNext();
        }
        
        // Закрываем все файлы и удаляем временные
        output.close();
        for (size_t i = 0; i < tempFileCount; ++i) {
            std::string tempFile = createTempFile(i);
            files[i].file.close();
            std::remove(tempFile.c_str());
        }
        
        return true;
    }

public:
    FileSorter(const std::string& input, const std::string& output)
        : inputPath(input), outputPath(output) {}

    bool sort() {
        std::ifstream input(inputPath);
        if (!input) {
            std::cerr << "Ошибка: не удалось открыть входной файл " << inputPath << std::endl;
            return false;
        }
        
        size_t tempFileCount = 0;
        size_t linesProcessed = 0;
        
        // Обрабатываем файл по частям
        while (sortBatch(input, tempFileCount, linesProcessed)) {
            tempFileCount++;
        }
        
        input.close();
        
        // Если не было создано временных файлов, значит входной файл пуст
        if (tempFileCount == 0) {
            std::ofstream output(outputPath);
            if (!output) {
                std::cerr << "Ошибка: не удалось создать файл результата " << outputPath << std::endl;
                return false;
            }
            output.close();
            return true;
        }
        
        // Объединяем временные файлы
        return mergeTempFiles(tempFileCount);
    }
};

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <входной_файл> <выходной_файл>" << std::endl;
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    // Запускаем таймер
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Создаем и запускаем сортировщик
    FileSorter sorter(inputPath, outputPath);
    if (!sorter.sort()) {
        std::cerr << "Ошибка при сортировке файла" << std::endl;
        return 1;
    }

    // Останавливаем таймер и вычисляем затраченное время
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "Сортировка завершена успешно. Результат сохранен в " << outputPath << std::endl;
    std::cout << "Время выполнения: " << duration.count() << " мс" << std::endl;
    
    return 0;
}
