cmake_minimum_required(VERSION 3.16)
project(barberia_pos VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Opcion para depurar en Windows: muestra la consola con los mensajes del programa.
#   cmake -DBARBERIA_CONSOLE=ON ..
option(BARBERIA_CONSOLE "Windows: mostrar ventana de consola (depuracion)" OFF)

# Dependencias via pkg-config (Linux, MSYS2/MinGW, macOS)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)

# Archivos fuente (antes faltaba receipt_printer.cpp -> error de enlazado)
set(SOURCES
    src/main.cpp
    src/database.cpp
    src/csv_exporter.cpp
    src/receipt_printer.cpp
    src/product_dialog.cpp
    src/main_window.cpp
)

if(WIN32 AND NOT BARBERIA_CONSOLE)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE
    PkgConfig::GTKMM
    PkgConfig::SQLITE3
)

# Copiar style.css junto al ejecutable despues de compilar
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/src/style.css"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/style.css"
    COMMENT "Copiando style.css al directorio del ejecutable..."
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(FILES src/style.css DESTINATION bin)
