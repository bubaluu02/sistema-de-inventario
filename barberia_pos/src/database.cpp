#include "database.hpp"
#include <iostream>
#include <sstream>

Database& Database::instance() {
    static Database db;
    return db;
}

Database::~Database() { close(); }

bool Database::open(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dbPath = dbPath;
    if (m_db) { sqlite3_close(m_db); m_db = nullptr; }
    if (sqlite3_open(m_dbPath.c_str(), &m_db) != SQLITE_OK) {
        std::cerr << "Error DB: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    char* err = nullptr;
    sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    if (!initSchema()) return false;
    seedInitialDataIfEmpty();
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) { sqlite3_close(m_db); m_db = nullptr; }
}

bool Database::initSchema() {
    const char* schema = R"SQL(
        CREATE TABLE IF NOT EXISTS products (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sku TEXT UNIQUE NOT NULL,
            name TEXT NOT NULL,
            brand TEXT DEFAULT '',
            category TEXT NOT NULL,
            cost_price REAL NOT NULL,
            sale_price REAL NOT NULL,
            stock INTEGER NOT NULL DEFAULT 0,
            min_stock INTEGER NOT NULL DEFAULT 5,
            is_active INTEGER NOT NULL DEFAULT 1,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        CREATE TABLE IF NOT EXISTS sales (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sale_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            total_revenue REAL NOT NULL,
            total_cost REAL NOT NULL,
            total_profit REAL NOT NULL,
            discount REAL DEFAULT 0.0,
            payment_method TEXT DEFAULT 'Efectivo',
            notes TEXT
        );
        CREATE TABLE IF NOT EXISTS sale_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sale_id INTEGER NOT NULL,
            product_id INTEGER NOT NULL,
            quantity INTEGER NOT NULL,
            unit_cost REAL NOT NULL,
            unit_price REAL NOT NULL,
            subtotal REAL NOT NULL,
            profit REAL NOT NULL,
            FOREIGN KEY(sale_id) REFERENCES sales(id) ON DELETE CASCADE,
            FOREIGN KEY(product_id) REFERENCES products(id) ON DELETE RESTRICT
        );
    )SQL";
    char* err = nullptr;
    int rc = sqlite3_exec(m_db, schema, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "Schema error: " << (err ? err : "unknown") << std::endl;
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

void Database::seedInitialDataIfEmpty() {
    // Left empty intentionally so user has clean slate to add their own inventory and prices
}

// ─────────── Products CRUD ───────────

std::vector<Product> Database::getProducts(const std::string& filter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Product> list;
    if (!m_db) return list;
    std::string sql = "SELECT id,sku,name,brand,category,cost_price,sale_price,stock,min_stock,created_at FROM products WHERE is_active=1";
    if (!filter.empty()) sql += " AND (name LIKE ? OR sku LIKE ? OR category LIKE ? OR brand LIKE ?)";
    sql += " ORDER BY name ASC;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
        if (!filter.empty()) {
            std::string f = "%" + filter + "%";
            for (int i = 1; i <= 4; ++i)
                sqlite3_bind_text(st, i, f.c_str(), -1, SQLITE_TRANSIENT);
        }
        while (sqlite3_step(st) == SQLITE_ROW) {
            Product p;
            p.id = sqlite3_column_int(st, 0);
            p.sku = (const char*)sqlite3_column_text(st, 1);
            p.name = (const char*)sqlite3_column_text(st, 2);
            auto bTxt = sqlite3_column_text(st, 3);
            p.brand = bTxt ? (const char*)bTxt : "";
            p.category = (const char*)sqlite3_column_text(st, 4);
            p.cost_price = sqlite3_column_double(st, 5);
            p.sale_price = sqlite3_column_double(st, 6);
            p.stock = sqlite3_column_int(st, 7);
            p.min_stock = sqlite3_column_int(st, 8);
            auto cTxt = sqlite3_column_text(st, 9);
            p.created_at = cTxt ? (const char*)cTxt : "";
            list.push_back(p);
        }
        sqlite3_finalize(st);
    }
    return list;
}

std::optional<Product> Database::getProductById(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) return std::nullopt;
    const char* sql = "SELECT id,sku,name,brand,category,cost_price,sale_price,stock,min_stock FROM products WHERE id=? AND is_active=1;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(st, 1, id);
        if (sqlite3_step(st) == SQLITE_ROW) {
            Product p;
            p.id = sqlite3_column_int(st, 0);
            p.sku = (const char*)sqlite3_column_text(st, 1);
            p.name = (const char*)sqlite3_column_text(st, 2);
            auto bT = sqlite3_column_text(st, 3);
            p.brand = bT ? (const char*)bT : "";
            p.category = (const char*)sqlite3_column_text(st, 4);
            p.cost_price = sqlite3_column_double(st, 5);
            p.sale_price = sqlite3_column_double(st, 6);
            p.stock = sqlite3_column_int(st, 7);
            p.min_stock = sqlite3_column_int(st, 8);
            sqlite3_finalize(st);
            return p;
        }
        sqlite3_finalize(st);
    }
    return std::nullopt;
}

bool Database::addProduct(const Product& p, int& outId, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) { errorMsg = "BD no inicializada."; return false; }
    const char* sql = "INSERT INTO products (sku,name,brand,category,cost_price,sale_price,stock,min_stock,is_active) VALUES (?,?,?,?,?,?,?,?,1);";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) != SQLITE_OK) { errorMsg = sqlite3_errmsg(m_db); return false; }
    sqlite3_bind_text(st, 1, p.sku.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, p.brand.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, p.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st, 5, p.cost_price);
    sqlite3_bind_double(st, 6, p.sale_price);
    sqlite3_bind_int(st, 7, p.stock);
    sqlite3_bind_int(st, 8, p.min_stock);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) { errorMsg = sqlite3_errmsg(m_db); return false; }
    outId = (int)sqlite3_last_insert_rowid(m_db);
    return true;
}

bool Database::updateProduct(const Product& p, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) { errorMsg = "BD no inicializada."; return false; }
    const char* sql = "UPDATE products SET sku=?,name=?,brand=?,category=?,cost_price=?,sale_price=?,stock=?,min_stock=? WHERE id=?;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) != SQLITE_OK) { errorMsg = sqlite3_errmsg(m_db); return false; }
    sqlite3_bind_text(st, 1, p.sku.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, p.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, p.brand.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, p.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st, 5, p.cost_price);
    sqlite3_bind_double(st, 6, p.sale_price);
    sqlite3_bind_int(st, 7, p.stock);
    sqlite3_bind_int(st, 8, p.min_stock);
    sqlite3_bind_int(st, 9, p.id);
    int rc = sqlite3_step(st);
    sqlite3_finalize(st);
    if (rc != SQLITE_DONE) { errorMsg = sqlite3_errmsg(m_db); return false; }
    return true;
}

bool Database::deleteProduct(int id, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) { errorMsg = "BD no inicializada."; return false; }
    // Check sales history
    int salesCount = 0;
    sqlite3_stmt* ck = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT COUNT(*) FROM sale_items WHERE product_id=?;", -1, &ck, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(ck, 1, id);
        if (sqlite3_step(ck) == SQLITE_ROW) salesCount = sqlite3_column_int(ck, 0);
        sqlite3_finalize(ck);
    }
    if (salesCount > 0) {
        // Soft delete
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, "UPDATE products SET is_active=0 WHERE id=?;", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, id);
            sqlite3_step(s);
            sqlite3_finalize(s);
            return true;
        }
        errorMsg = sqlite3_errmsg(m_db);
        return false;
    }
    sqlite3_stmt* d = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM products WHERE id=?;", -1, &d, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(d, 1, id);
        int rc = sqlite3_step(d);
        sqlite3_finalize(d);
        if (rc != SQLITE_DONE) { errorMsg = sqlite3_errmsg(m_db); return false; }
        return true;
    }
    errorMsg = sqlite3_errmsg(m_db);
    return false;
}

std::vector<std::string> Database::getCategories() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> cats;
    if (!m_db) return cats;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, "SELECT DISTINCT category FROM products WHERE is_active=1 ORDER BY category;", -1, &st, nullptr) == SQLITE_OK) {
        while (sqlite3_step(st) == SQLITE_ROW)
            cats.emplace_back((const char*)sqlite3_column_text(st, 0));
        sqlite3_finalize(st);
    }
    return cats;
}

// ─────────── Sales ───────────

bool Database::recordSale(const std::vector<CartItem>& items, const std::string& notes,
                          double discount, const std::string& paymentMethod,
                          int& outSaleId, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) { errorMsg = "BD no conectada."; return false; }
    if (items.empty()) { errorMsg = "Ticket vacío."; return false; }

    // Verify stock
    for (const auto& item : items) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(m_db, "SELECT stock,name FROM products WHERE id=? AND is_active=1;", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, item.product.id);
            if (sqlite3_step(s) == SQLITE_ROW) {
                int curStock = sqlite3_column_int(s, 0);
                std::string pName = (const char*)sqlite3_column_text(s, 1);
                sqlite3_finalize(s);
                if (curStock < item.quantity) {
                    errorMsg = "Stock insuficiente: '" + pName + "' (disponible: " + std::to_string(curStock) + ", pedido: " + std::to_string(item.quantity) + ")";
                    return false;
                }
            } else { sqlite3_finalize(s); errorMsg = "Producto no encontrado."; return false; }
        }
    }

    double totalRev = 0, totalCost = 0;
    for (const auto& item : items) { totalRev += item.subtotal(); totalCost += item.total_cost(); }
    double totalProfit = totalRev - totalCost - discount;

    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    sqlite3_stmt* saleStmt = nullptr;
    if (sqlite3_prepare_v2(m_db, "INSERT INTO sales (total_revenue,total_cost,total_profit,discount,payment_method,notes) VALUES (?,?,?,?,?,?);",
                           -1, &saleStmt, nullptr) != SQLITE_OK) {
        errorMsg = sqlite3_errmsg(m_db);
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_double(saleStmt, 1, totalRev);
    sqlite3_bind_double(saleStmt, 2, totalCost);
    sqlite3_bind_double(saleStmt, 3, totalProfit);
    sqlite3_bind_double(saleStmt, 4, discount);
    sqlite3_bind_text(saleStmt, 5, paymentMethod.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(saleStmt, 6, notes.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(saleStmt) != SQLITE_DONE) {
        errorMsg = sqlite3_errmsg(m_db);
        sqlite3_finalize(saleStmt);
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_finalize(saleStmt);
    outSaleId = (int)sqlite3_last_insert_rowid(m_db);

    sqlite3_stmt *itSt = nullptr, *upSt = nullptr;
    sqlite3_prepare_v2(m_db, "INSERT INTO sale_items (sale_id,product_id,quantity,unit_cost,unit_price,subtotal,profit) VALUES (?,?,?,?,?,?,?);", -1, &itSt, nullptr);
    sqlite3_prepare_v2(m_db, "UPDATE products SET stock=stock-? WHERE id=?;", -1, &upSt, nullptr);

    for (const auto& item : items) {
        sqlite3_bind_int(itSt, 1, outSaleId);
        sqlite3_bind_int(itSt, 2, item.product.id);
        sqlite3_bind_int(itSt, 3, item.quantity);
        sqlite3_bind_double(itSt, 4, item.product.cost_price);
        sqlite3_bind_double(itSt, 5, item.product.sale_price);
        sqlite3_bind_double(itSt, 6, item.subtotal());
        sqlite3_bind_double(itSt, 7, item.profit());
        sqlite3_step(itSt); sqlite3_reset(itSt);

        sqlite3_bind_int(upSt, 1, item.quantity);
        sqlite3_bind_int(upSt, 2, item.product.id);
        sqlite3_step(upSt); sqlite3_reset(upSt);
    }
    sqlite3_finalize(itSt);
    sqlite3_finalize(upSt);

    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}

bool Database::deleteSale(int saleId, bool restoreStock, std::string& errorMsg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_db) { errorMsg = "BD no conectada."; return false; }

    sqlite3_exec(m_db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    if (restoreStock) {
        // Return products to inventory
        sqlite3_stmt* itSt = nullptr;
        const char* qItems = "SELECT product_id, quantity FROM sale_items WHERE sale_id=?;";
        if (sqlite3_prepare_v2(m_db, qItems, -1, &itSt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(itSt, 1, saleId);

            sqlite3_stmt* upSt = nullptr;
            const char* upSql = "UPDATE products SET stock=stock+? WHERE id=?;";
            sqlite3_prepare_v2(m_db, upSql, -1, &upSt, nullptr);

            while (sqlite3_step(itSt) == SQLITE_ROW) {
                int pId = sqlite3_column_int(itSt, 0);
                int qty = sqlite3_column_int(itSt, 1);
                if (upSt) {
                    sqlite3_bind_int(upSt, 1, qty);
                    sqlite3_bind_int(upSt, 2, pId);
                    sqlite3_step(upSt);
                    sqlite3_reset(upSt);
                }
            }
            if (upSt) sqlite3_finalize(upSt);
            sqlite3_finalize(itSt);
        }
    }

    // Delete sale_items first
    sqlite3_stmt* delItems = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM sale_items WHERE sale_id=?;", -1, &delItems, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(delItems, 1, saleId);
        sqlite3_step(delItems);
        sqlite3_finalize(delItems);
    }

    // Delete sale
    sqlite3_stmt* delSale = nullptr;
    if (sqlite3_prepare_v2(m_db, "DELETE FROM sales WHERE id=?;", -1, &delSale, nullptr) != SQLITE_OK) {
        errorMsg = sqlite3_errmsg(m_db);
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }
    sqlite3_bind_int(delSale, 1, saleId);
    int rc = sqlite3_step(delSale);
    sqlite3_finalize(delSale);

    if (rc != SQLITE_DONE) {
        errorMsg = sqlite3_errmsg(m_db);
        sqlite3_exec(m_db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return false;
    }

    sqlite3_exec(m_db, "COMMIT;", nullptr, nullptr, nullptr);
    return true;
}

// ─────────── Reporting ───────────

BusinessMetrics Database::getMetrics(TimeFilter tf) {
    std::lock_guard<std::mutex> lock(m_mutex);
    BusinessMetrics m;
    if (!m_db) return m;
    std::string tfSql = timeFilterToSQL(tf);

    // Sales totals
    {
        std::string sql = "SELECT COALESCE(SUM(total_revenue),0),COALESCE(SUM(total_cost),0),COALESCE(SUM(total_profit),0),COUNT(*) FROM sales s WHERE 1=1" + tfSql + ";";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) {
                m.total_revenue = sqlite3_column_double(st, 0);
                m.total_cost = sqlite3_column_double(st, 1);
                m.net_profit = sqlite3_column_double(st, 2);
                m.total_sales_count = sqlite3_column_int(st, 3);
            }
            sqlite3_finalize(st);
        }
    }
    if (m.total_revenue > 0) m.average_margin = (m.net_profit / m.total_revenue) * 100.0;
    if (m.total_sales_count > 0) m.average_ticket = m.total_revenue / m.total_sales_count;

    // Units sold
    {
        std::string sql = "SELECT COALESCE(SUM(si.quantity),0) FROM sale_items si JOIN sales s ON si.sale_id=s.id WHERE 1=1" + tfSql + ";";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) m.total_units_sold = sqlite3_column_int(st, 0);
            sqlite3_finalize(st);
        }
    }

    // Top product
    {
        std::string sql = "SELECT p.name,SUM(si.quantity) as uds,SUM(si.subtotal) as rev FROM sale_items si JOIN sales s ON si.sale_id=s.id JOIN products p ON si.product_id=p.id WHERE 1=1" + tfSql + " GROUP BY p.id ORDER BY uds DESC LIMIT 1;";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) {
                auto txt = sqlite3_column_text(st, 0);
                m.top_product_name = txt ? (const char*)txt : "N/A";
                m.top_product_units = sqlite3_column_int(st, 1);
                m.top_product_revenue = sqlite3_column_double(st, 2);
            }
            sqlite3_finalize(st);
        }
    }

    // Inventory
    {
        const char* sql = "SELECT COUNT(*),COALESCE(SUM(CASE WHEN stock<=min_stock AND stock>0 THEN 1 ELSE 0 END),0),COALESCE(SUM(CASE WHEN stock<=0 THEN 1 ELSE 0 END),0),COALESCE(SUM(stock*cost_price),0),COALESCE(SUM(stock*sale_price),0) FROM products WHERE is_active=1;";
        sqlite3_stmt* st = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) == SQLITE_OK) {
            if (sqlite3_step(st) == SQLITE_ROW) {
                m.total_products_in_catalog = sqlite3_column_int(st, 0);
                m.low_stock_count = sqlite3_column_int(st, 1);
                m.out_of_stock_count = sqlite3_column_int(st, 2);
                m.inventory_cost_value = sqlite3_column_double(st, 3);
                m.inventory_sale_value = sqlite3_column_double(st, 4);
            }
            sqlite3_finalize(st);
        }
        m.inventory_potential_profit = m.inventory_sale_value - m.inventory_cost_value;
    }
    return m;
}

std::vector<ProductSalesStat> Database::getProductSalesBreakdown(const std::string& filter, TimeFilter tf) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ProductSalesStat> stats;
    if (!m_db) return stats;
    std::string tfSql = timeFilterToSQL(tf);

    std::string sql = "SELECT p.id,p.sku,p.name,p.brand,p.category,COALESCE(SUM(si.quantity),0),COALESCE(SUM(si.subtotal),0),COALESCE(SUM(si.quantity*si.unit_cost),0),COALESCE(SUM(si.profit),0),p.stock "
                      "FROM products p LEFT JOIN (sale_items si JOIN sales s ON si.sale_id=s.id " + (tfSql.empty() ? std::string("") : "AND 1=1" + tfSql) + ") ON p.id=si.product_id "
                      "WHERE p.is_active=1";
    if (!filter.empty()) sql += " AND (p.name LIKE ? OR p.sku LIKE ? OR p.category LIKE ? OR p.brand LIKE ?)";
    sql += " GROUP BY p.id ORDER BY 6 DESC,9 DESC,p.name ASC;";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
        if (!filter.empty()) {
            std::string f = "%" + filter + "%";
            for (int i = 1; i <= 4; ++i) sqlite3_bind_text(st, i, f.c_str(), -1, SQLITE_TRANSIENT);
        }
        while (sqlite3_step(st) == SQLITE_ROW) {
            ProductSalesStat s;
            s.product_id = sqlite3_column_int(st, 0);
            s.sku = (const char*)sqlite3_column_text(st, 1);
            s.name = (const char*)sqlite3_column_text(st, 2);
            auto bT = sqlite3_column_text(st, 3);
            s.brand = bT ? (const char*)bT : "";
            s.category = (const char*)sqlite3_column_text(st, 4);
            s.units_sold = sqlite3_column_int(st, 5);
            s.total_revenue = sqlite3_column_double(st, 6);
            s.total_cost = sqlite3_column_double(st, 7);
            s.total_profit = sqlite3_column_double(st, 8);
            s.current_stock = sqlite3_column_int(st, 9);
            s.margin_percent = s.total_revenue > 0 ? (s.total_profit / s.total_revenue) * 100.0 : 0.0;
            stats.push_back(s);
        }
        sqlite3_finalize(st);
    }
    return stats;
}

std::vector<SaleRecord> Database::getSalesHistory(TimeFilter tf) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SaleRecord> list;
    if (!m_db) return list;
    std::string tfSql = timeFilterToSQL(tf);

    std::string sql = "SELECT s.id,s.sale_date,s.total_revenue,s.total_cost,s.total_profit,COALESCE(s.notes,''),s.discount,s.payment_method,"
                      "COALESCE(SUM(si.quantity),0),GROUP_CONCAT(p.name||' ('||si.quantity||')',', ') "
                      "FROM sales s LEFT JOIN sale_items si ON s.id=si.sale_id LEFT JOIN products p ON si.product_id=p.id "
                      "WHERE 1=1" + tfSql + " GROUP BY s.id ORDER BY s.sale_date DESC;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &st, nullptr) == SQLITE_OK) {
        while (sqlite3_step(st) == SQLITE_ROW) {
            SaleRecord r;
            r.id = sqlite3_column_int(st, 0);
            r.date_time = (const char*)sqlite3_column_text(st, 1);
            r.total_revenue = sqlite3_column_double(st, 2);
            r.total_cost = sqlite3_column_double(st, 3);
            r.total_profit = sqlite3_column_double(st, 4);
            r.notes = (const char*)sqlite3_column_text(st, 5);
            r.discount = sqlite3_column_double(st, 6);
            auto pm = sqlite3_column_text(st, 7);
            r.payment_method = pm ? (const char*)pm : "Efectivo";
            r.total_items = sqlite3_column_int(st, 8);
            auto its = sqlite3_column_text(st, 9);
            r.items_summary = its ? (const char*)its : "";
            list.push_back(r);
        }
        sqlite3_finalize(st);
    }
    return list;
}

std::vector<SaleItemDetail> Database::getSaleDetails(int saleId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SaleItemDetail> items;
    if (!m_db) return items;
    const char* sql = "SELECT p.sku,p.name,si.quantity,si.unit_price,si.subtotal,si.profit "
                      "FROM sale_items si JOIN products p ON si.product_id=p.id WHERE si.sale_id=?;";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(st, 1, saleId);
        while (sqlite3_step(st) == SQLITE_ROW) {
            SaleItemDetail d;
            d.sku = (const char*)sqlite3_column_text(st, 0);
            d.product_name = (const char*)sqlite3_column_text(st, 1);
            d.quantity = sqlite3_column_int(st, 2);
            d.unit_price = sqlite3_column_double(st, 3);
            d.subtotal = sqlite3_column_double(st, 4);
            d.profit = sqlite3_column_double(st, 5);
            items.push_back(d);
        }
        sqlite3_finalize(st);
    }
    return items;
}
