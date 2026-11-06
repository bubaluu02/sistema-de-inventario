CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 $(shell pkg-config --cflags gtkmm-4.0) -I./src
LDFLAGS = $(shell pkg-config --libs gtkmm-4.0) -lsqlite3

SRC_DIR = src
BUILD_DIR = build
TARGET = barberia_pos

SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/database.cpp \
       $(SRC_DIR)/csv_exporter.cpp \
       $(SRC_DIR)/receipt_printer.cpp \
       $(SRC_DIR)/product_dialog.cpp \
       $(SRC_DIR)/main_window.cpp

OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

.PHONY: all clean run reset-db

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "===> Vinculando $(TARGET)..."
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	@echo "===> Compilación exitosa: ./$(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "===> Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "===> Ejecutando $(TARGET)..."
	./$(TARGET)

reset-db:
	@echo "===> Restableciendo base de datos SQLite..."
	rm -f barberia.db
	@echo "===> Base de datos eliminada. Se re-inicializará en la próxima ejecución."

clean:
	@echo "===> Limpiando archivos objeto y binarios..."
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "===> Limpieza terminada."
