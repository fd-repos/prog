# Компилятор и флаги
CXX      := g++
CXXFLAGS := -std=c++17 -O3 -march=native -flto -fuse-linker-plugin

# Имя исполняемого файла
TARGET := sort_bigdatafile

# Исходные файлы
SRCS := sort_bigdatafile.cpp

# Объектные файлы
OBJS := $(SRCS:.cpp=.o)

# Флаги отладки и релиза
DEBUG_FLAGS := -g -DDEBUG
RELEASE_FLAGS := -DNDEBUG

# Цели сборки
.PHONY: all del debug release help go

# По умолчанию собираем релизную версию
all:
	make -j$(shell nproc) release

# Релизная сборка
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# Отладочная сборка
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Правило сборки исполняемого файла
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Правило компиляции .cpp в .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $^

# Запуск генератора
gen:
	python gen_bigdatafile.py

# Запуск с именами по умолчанию и в едином общем каталоге
sort:
	touch sort_data.txt; \
	./sort_bigdatafile ./gen_data.txt ./sort_data.txt;

# Очистка собранных файлов
del:
	rm -f $(TARGET) $(OBJS)

# Вывод помощи
help:
	@echo ""
	@echo "доступные команды"
	@echo "================="
	@echo "  make       | собрать релизную версию"
	@echo "  make debug | собрать отладочную версию"
	@echo "  make gen   | создать файл gen_data.txt"
	@echo "  make sort  | запуск с параметрами по умолчанию sort_bigdatafile c"
	@echo "  make del | очистить собранные файлы"
	@echo "  make help  | показать эту справку"
	@echo ""