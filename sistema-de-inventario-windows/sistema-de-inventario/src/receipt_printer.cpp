#include "receipt_printer.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
  #include <shellapi.h>
#endif

namespace fs = std::filesystem;

static std::string centerText(const std::string& text, int width) {
    if ((int)text.size() >= width) return text;
    int pad = (width - (int)text.size()) / 2;
    return std::string(pad, ' ') + text;
}

static std::string repeatChar(char c, int n) {
    return std::string(n, c);
}

std::string ReceiptPrinter::generateReceipt(int saleId, const std::string& dateTime,
                                            const std::vector<CartItem>& items,
                                            double discount, double totalPaid,
                                            double cashReceived, double change,
                                            const std::string& paymentMethod,
                                            double exchangeRate) {
    const int W = 48; // Receipt width (standard thermal printer)
    std::ostringstream r;
    std::string sep = repeatChar('=', W);
    std::string sep2 = repeatChar('-', W);

    r << sep << "\n";
    r << centerText("BARBERIA PRO", W) << "\n";
    r << centerText("Sistema de Ventas e Inventario", W) << "\n";
    r << centerText("Honduras, C.A.", W) << "\n";
    r << sep << "\n";

    r << " Ticket #: " << std::setw(6) << std::setfill('0') << saleId << std::setfill(' ') << "\n";
    r << " Fecha:    " << dateTime << "\n";
    r << " Pago:     " << paymentMethod << "\n";
    r << sep2 << "\n";

    // Header
    r << " " << std::left << std::setw(3) << "Ud"
      << " " << std::setw(22) << "Producto"
      << " " << std::right << std::setw(9) << "P.Unit."
      << " " << std::setw(10) << "Subtotal" << "\n";
    r << sep2 << "\n";

    double subtotalGeneral = 0.0;
    for (const auto& item : items) {
        std::string nombre = item.product.name;
        if (nombre.size() > 22) nombre = nombre.substr(0, 20) + "..";

        r << " " << std::left << std::setw(3) << item.quantity
          << " " << std::setw(22) << nombre
          << " " << std::right << std::setw(9) << std::fixed << std::setprecision(2) << item.product.sale_price
          << " " << std::setw(10) << std::fixed << std::setprecision(2) << item.subtotal() << "\n";
        subtotalGeneral += item.subtotal();
    }

    r << sep2 << "\n";
    r << std::right << std::setw(W - 14) << "SUBTOTAL: L " << std::fixed << std::setprecision(2) << std::setw(12) << subtotalGeneral << "\n";

    if (discount > 0.0) {
        r << std::right << std::setw(W - 14) << "DESCUENTO: L " << std::fixed << std::setprecision(2) << std::setw(11) << -discount << "\n";
    }

    r << sep << "\n";
    r << std::right << std::setw(W - 14) << "TOTAL: L " << std::fixed << std::setprecision(2) << std::setw(15) << totalPaid << "\n";

    if (exchangeRate > 0.0) {
        double totalUSD = totalPaid / exchangeRate;
        r << std::right << std::setw(W - 14) << "TOTAL USD: $ " << std::fixed << std::setprecision(2) << std::setw(11) << totalUSD << "\n";
        r << std::right << std::setw(W - 14) << "(Tipo cambio: " << std::fixed << std::setprecision(2) << std::setw(10) << exchangeRate << ")\n";
    }
    r << sep << "\n";

    if (paymentMethod == "Efectivo" && cashReceived > 0.0) {
        r << std::right << std::setw(W - 14) << "EFECTIVO: L " << std::fixed << std::setprecision(2) << std::setw(12) << cashReceived << "\n";
        r << std::right << std::setw(W - 14) << "CAMBIO: L " << std::fixed << std::setprecision(2) << std::setw(14) << change << "\n";
        r << sep2 << "\n";
    }

    r << "\n";
    r << centerText("Gracias por su preferencia!", W) << "\n";
    r << centerText("Conserve este ticket para su control", W) << "\n";
    r << centerText("www.barberiapro.hn", W) << "\n";
    r << "\n" << sep << "\n";

    return r.str();
}

void ReceiptPrinter::ensureDirectoryExists(const std::string& directory) {
    // std::filesystem funciona igual en Linux y Windows (mkdir() de POSIX no).
    std::error_code ec;
    fs::create_directories(fs::u8path(directory), ec);
}

bool ReceiptPrinter::saveReceiptToFile(const std::string& receipt, const std::string& directory, int saleId) {
    ensureDirectoryExists(directory);

    fs::path filename = fs::u8path(directory) / ("ticket_" + std::to_string(saleId) + ".txt");
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    file << receipt;
    file.close();
    return true;
}

#ifdef _WIN32

// ------------------------- WINDOWS -------------------------

static bool shellOk(HINSTANCE h) {
    // ShellExecute devuelve un valor > 32 cuando tuvo exito.
    return reinterpret_cast<INT_PTR>(h) > 32;
}

bool ReceiptPrinter::printReceiptFile(const std::string& filePath) {
    std::error_code ec;
    fs::path abs = fs::absolute(fs::u8path(filePath), ec);
    if (ec || !fs::exists(abs)) return false;

    // 1) Verbo "print" de Windows: manda el .txt a la impresora predeterminada.
    HINSTANCE r = ShellExecuteW(nullptr, L"print", abs.wstring().c_str(),
                                nullptr, nullptr, SW_HIDE);
    if (shellOk(r)) return true;

    // 2) Respaldo: Bloc de notas en modo impresion.
    std::wstring args = L"/p \"" + abs.wstring() + L"\"";
    r = ShellExecuteW(nullptr, L"open", L"notepad.exe", args.c_str(),
                      nullptr, SW_HIDE);
    return shellOk(r);
}

bool ReceiptPrinter::printReceipt(const std::string& receipt) {
    // En Windows no hay "lp": se guarda en un archivo temporal y se imprime.
    std::error_code ec;
    fs::path tmp = fs::temp_directory_path(ec) / "barberia_ticket_tmp.txt";
    if (ec) return false;
    {
        std::ofstream f(tmp, std::ios::binary);
        if (!f.is_open()) return false;
        f << receipt;
    }
    return printReceiptFile(tmp.u8string());
}

bool ReceiptPrinter::openReceiptFolder(const std::string& directory) {
    ensureDirectoryExists(directory);
    std::error_code ec;
    fs::path abs = fs::absolute(fs::u8path(directory), ec);
    if (ec) return false;
    HINSTANCE r = ShellExecuteW(nullptr, L"open", abs.wstring().c_str(),
                                nullptr, nullptr, SW_SHOWNORMAL);
    return shellOk(r);
}

#else

// -------------------------- LINUX --------------------------

bool ReceiptPrinter::printReceipt(const std::string& receipt) {
    FILE* pipe = popen("lp 2>/dev/null", "w");
    if (!pipe) return false;
    fwrite(receipt.c_str(), 1, receipt.size(), pipe);
    int result = pclose(pipe);
    return (result == 0);
}

bool ReceiptPrinter::printReceiptFile(const std::string& filePath) {
    std::string cmd = "lp \"" + filePath + "\" 2>/dev/null";
    int res = std::system(cmd.c_str());
    return (res == 0);
}

bool ReceiptPrinter::openReceiptFolder(const std::string& directory) {
    ensureDirectoryExists(directory);
    std::string cmd = "xdg-open \"" + directory + "\" >/dev/null 2>&1 &";
    int res = std::system(cmd.c_str());
    return (res == 0);
}

#endif
