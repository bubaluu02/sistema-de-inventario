Sistema de Gestión, Ventas e Inventario (C++ / SQLite / GTK4)

Sistema de punto de venta (POS), facturación, inventario y control financiero diseñado para barberías y salones masculinos. Desarrollado en **C++17** con interfaz estilo hoja de cálculo (**GTK4 / gtkmm-4.0**) y base de datos relacional integrada (**SQLite3**).

---

Características

- 📊 **Control Financiero & Ganancias**: Visualiza ingresos, costos de ventas, ganancia neta real y margen porcentual en tiempo real.
- 🇭🇳 **Multi-moneda (Lempiras HNL & Dólares USD)**: Tipo de cambio editable en vivo desde la barra principal.
- 📦 **Gestión Completa de Inventario**: Agregar, editar y dar de baja productos con alertas de stock mínimo y cálculo de margen unitario en tiempo real.
- 🛒 **Punto de Venta (POS)**: Registro de ventas con ticket dinámico, cálculo de cambio/vuelto, descuentos y formas de pago (Efectivo, Tarjeta, Transferencia).
- 🧾 **Generación e Impresión de Tickets**: Creación automática de tickets en formato texto en la carpeta `recibos/` y soporte para impresión directa con un clic.
- 🗑️ **Eliminación Segura de Ventas**: Opción de eliminar tickets y revertir o conservar el stock de productos.
- 📥 **Exportación a Excel**: Exporta reportes financieros, inventario e historial a formato `.csv` compatible con Excel y LibreOffice.

---

## 🐧 Instalación y Ejecución en Linux

### 1. Requisitos previos (Arch Linux / Manjaro)
```bash
sudo pacman -S base-devel gcc make sqlite gtkmm-4.0
```

### Requisitos previos (Ubuntu / Debian / Linux Mint)
```bash
sudo apt update
sudo apt install build-essential libgtkmm-4.0-dev libsqlite3-dev
```

### Requisitos previos (Fedora)
```bash
sudo dnf install gcc-c++ make gtkmm4.0-devel sqlite-devel
```

### 2. Compilar
Abre una terminal dentro de la carpeta del proyecto y ejecuta:
```bash
make
```

### 3. Ejecutar
```bash
./barberia_pos
# o bien:
make run
```

---

## 🪟 Instalación y Ejecución en Windows

### Opción rápida (recomendada)

1. Instala **MSYS2** desde https://www.msys2.org (ruta por defecto `C:\msys64`).
2. Haz **doble clic en `compilar_windows.bat`**. El script:
   - instala el compilador, GTK4 y SQLite automáticamente,
   - compila el programa,
   - crea la carpeta **`dist\`** con `barberia_pos.exe`, todas las DLL y los recursos de GTK.
3. Abre `dist\barberia_pos.exe`. **Ya no necesita MSYS2**: puedes copiar la carpeta `dist\` completa a cualquier PC con Windows.

### Notas

- La base de datos (`barberia.db`), los tickets (`recibos\`) y los CSV exportados se guardan **junto al `.exe`**, sin importar desde dónde lo abras.
- **Imprimir tickets**: se envía a la impresora predeterminada de Windows (Bloc de notas / verbo "print"). Si falla, el ticket queda guardado en `recibos\`.
- **Depuración**: para ver los mensajes del programa en una consola, compila con `cmake -DBARBERIA_CONSOLE=ON ..`.
- Si el `.exe` se cierra solo, ejecútalo desde una terminal y revisa el mensaje de error.

### Compilación manual (terminal "MSYS2 MinGW x64")

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-pkgconf mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-gtkmm-4.0 mingw-w64-x86_64-sqlite3
cd /c/Ruta/A/sistema-de-inventario
./compilar_windows.sh
```

---

## 📁 Estructura del Proyecto

```
sistema-de-inventario/
├── CMakeLists.txt           # Configuración para CMake (Multiplataforma)
├── Makefile                 # Script de compilación rápida para Linux
├── compilar_windows.bat     # Doble clic: compila y crea dist\ en Windows
├── compilar_windows.sh      # Script que ejecuta el .bat dentro de MSYS2
├── README.md                # Guía de instalación y manual de uso
├── barberia.db              # Base de datos SQLite (se genera automáticamente)
├── recibos/                 # Carpeta donde se guardan los tickets (.txt)
└── src/
    ├── main.cpp             # Punto de entrada de la aplicación
    ├── models.hpp           # Estructuras de datos (Productos, Ventas, Métricas)
    ├── database.hpp / .cpp  # Motor SQLite, transacciones y consultas
    ├── main_window.hpp/.cpp # Ventana principal, pestañas Excel y Punto de Venta
    ├── product_dialog.hpp/.cpp # Diálogo modal para agregar/editar productos
    ├── receipt_printer.hpp/.cpp # Generador y gestor de tickets para impresión
    ├── csv_exporter.hpp/.cpp # Exportador de reportes compatibles con Excel
    └── style.css            # Hoja de estilos GTK (tema verde Excel adaptable)
```

---

## 💡 Consejos de Uso
- **Cambio de moneda**: Puedes escribir la tasa de cambio del día (por ejemplo `24.75`) en la esquina superior derecha y todos los valores se adaptarán automáticamente entre Lempiras y Dólares.
- **Acceso a Tickets**: Pulsa el botón `🧾 Carpeta Recibos` en la barra superior para abrir la carpeta directamente desde tu explorador de archivos.
- **Reimpresión**: En la pestaña *Historial de Ventas*, haz doble clic en cualquier ticket para enviarlo a la impresora predeterminada.
