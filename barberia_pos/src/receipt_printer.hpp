#ifndef RECEIPT_PRINTER_HPP
#define RECEIPT_PRINTER_HPP

#include "models.hpp"
#include <string>
#include <vector>

class ReceiptPrinter {
public:
    static std::string generateReceipt(int saleId, const std::string& dateTime,
                                       const std::vector<CartItem>& items,
                                       double discount, double totalPaid,
                                       double cashReceived, double change,
                                       const std::string& paymentMethod,
                                       double exchangeRate);

    static bool saveReceiptToFile(const std::string& receipt, const std::string& directory, int saleId);
    static bool printReceipt(const std::string& receipt);
    static bool printReceiptFile(const std::string& filePath);
    static bool openReceiptFolder(const std::string& directory = "recibos");
    static void ensureDirectoryExists(const std::string& directory);
};

#endif // RECEIPT_PRINTER_HPP
