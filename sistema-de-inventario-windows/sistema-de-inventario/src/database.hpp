#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "models.hpp"
#include <sqlite3.h>
#include <vector>
#include <string>
#include <optional>
#include <mutex>

class Database {
public:
    static Database& instance();

    bool open(const std::string& dbPath = "barberia.db");
    void close();

    // Products CRUD
    std::vector<Product> getProducts(const std::string& filter = "");
    std::optional<Product> getProductById(int id);
    bool addProduct(const Product& p, int& outId, std::string& errorMsg);
    bool updateProduct(const Product& p, std::string& errorMsg);
    bool deleteProduct(int id, std::string& errorMsg);
    std::vector<std::string> getCategories();

    // Sales & POS
    bool recordSale(const std::vector<CartItem>& items, const std::string& notes,
                    double discount, const std::string& paymentMethod,
                    int& outSaleId, std::string& errorMsg);
    bool deleteSale(int saleId, bool restoreStock, std::string& errorMsg);

    // Reporting
    BusinessMetrics getMetrics(TimeFilter tf = TimeFilter::ALL_TIME);
    std::vector<ProductSalesStat> getProductSalesBreakdown(const std::string& filter = "", TimeFilter tf = TimeFilter::ALL_TIME);
    std::vector<SaleRecord> getSalesHistory(TimeFilter tf = TimeFilter::ALL_TIME);
    std::vector<SaleItemDetail> getSaleDetails(int saleId);

    std::string getDatabasePath() const { return m_dbPath; }

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool initSchema();
    void seedInitialDataIfEmpty();

    sqlite3* m_db = nullptr;
    std::string m_dbPath;
    std::mutex m_mutex;
};

#endif // DATABASE_HPP
