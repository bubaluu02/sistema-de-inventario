#!/usr/bin/env bash
# Se ejecuta DENTRO de MSYS2 MinGW64 (lo lanza compilar_windows.bat).
# 1) instala dependencias  2) compila  3) arma la carpeta dist\ lista para usar.
set -e
cd "$(dirname "$0")"

echo
echo "=== [1/3] Instalando dependencias (solo la primera vez tarda) ==="
pacman -S --needed --noconfirm \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-pkgconf \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-make \
    mingw-w64-x86_64-gtkmm-4.0 \
    mingw-w64-x86_64-sqlite3

echo
echo "=== [2/3] Compilando ==="
rm -rf build
mkdir -p build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j"$(nproc)"
cd ..

echo
echo "=== [3/3] Armando carpeta dist/ (programa + DLL + recursos de GTK) ==="
PREFIX="${MINGW_PREFIX:-/mingw64}"
EXE="build/barberia_pos.exe"
[ -f "$EXE" ] || { echo "[ERROR] No se genero $EXE"; exit 1; }

rm -rf dist
mkdir -p dist
cp "$EXE" dist/
cp src/style.css dist/

# DLL que necesita el .exe (ldd lista tambien las dependencias indirectas)
ldd "$EXE" | awk '/=>/ {print $3}' | grep -i "^$PREFIX/" | sort -u | while read -r dll; do
    cp -u "$dll" dist/
done

# Recursos de GTK4 / GLib (esquemas de configuracion e iconos)
mkdir -p dist/share/glib-2.0 dist/share/icons
cp -r "$PREFIX/share/glib-2.0/schemas" dist/share/glib-2.0/
for theme in Adwaita hicolor; do
    [ -d "$PREFIX/share/icons/$theme" ] && cp -r "$PREFIX/share/icons/$theme" dist/share/icons/
done
if [ -d "$PREFIX/lib/gdk-pixbuf-2.0" ]; then
    mkdir -p dist/lib
    cp -r "$PREFIX/lib/gdk-pixbuf-2.0" dist/lib/
fi

echo
echo "============================================================"
echo "  LISTO. Programa en:  dist/barberia_pos.exe"
echo "  Copia la carpeta dist/ completa a cualquier PC con Windows."
echo "  Haz doble clic en barberia_pos.exe (no necesita MSYS2)."
echo "============================================================"
