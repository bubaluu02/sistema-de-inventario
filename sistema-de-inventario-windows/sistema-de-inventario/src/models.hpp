#ifndef MODELS_HPP
#define MODELS_HPP

#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <ctime>
#include <chrono>

// ────────────────────────── Time Filtering ──────────────────────────

enum class TimeFilter {
    ALL_TIME,
    TODAY,
    THIS_WEEK,
    THIS_MONTH
};

inline std::string timeFilterToSQL(TimeFilter tf) {
    switch (tf) {
        case TimeFilter::TODAY:
            return " AND date(s.sale_date) = date('now','localtime') ";
        case TimeFilter::THIS_WEEK:
            return " AND s.sale_date >= datetime('now','localtime','-7 days') ";
        case TimeFilter::THIS_MONTH:
            return " AND s.sale_date >= datetime('now','localtime','start of month') ";
        default:
            return "";
    }
}

inline std::string timeFilterLabel(TimeFilter tf) {
    switch (tf) {
        case TimeFilter::TODAY:      return "Hoy";
        case TimeFilter::THIS_WEEK:  return "Esta Semana";
        case TimeFilter::THIS_MONTH: return "Este Mes";
        default:                     return "Todo el Historial";
    }
}

// ────────────────────────── Product ──────────────────────────

struct Product {
    int id = 0;
    std::string sku;
    std::string name;
    std::string brand;
    std::string category;
    double cost_price = 0.0;
    double sale_price = 0.0;
    int stock = 0;
    int min_stock = 5;
    std::string created_at;

    double unit_profit() const {
        return sale_price - cost_price;
    }

    double margin_percent() const {
        if (sale_price <= 0.0) return 0.0;
        return ((sale_price - cost_price) / sale_price) * 100.0;
    }

    double inventory_cost_value() const {
        return stock * cost_price;
    }

    double inventory_sale_value() const {
        return stock * sale_price;
    }

    bool is_low_stock() const {
        return stock > 0 && stock <= min_stock;
    }

    bool is_out_of_stock() const {
        return stock <= 0;
    }

    std::string stock_status_emoji() const {
        if (is_out_of_stock()) return "🔴 Agotado";
        if (is_low_stock())    return "🟡 Bajo Stock";
        return "🟢 Disponible";
    }
};

// ────────────────────────── Cart / POS ──────────────────────────

struct CartItem {
    Product product;
    int quantity = 1;

    double subtotal() const {
        return quantity * product.sale_price;
    }

    double total_cost() const {
        return quantity * product.cost_price;
    }

    double profit() const {
        return subtotal() - total_cost();
    }
};

// ────────────────────────── Sales Statistics ──────────────────────────

struct ProductSalesStat {
    int product_id = 0;
    std::string sku;
    std::string name;
    std::string brand;
    std::string category;
    int units_sold = 0;
    double total_revenue = 0.0;
    double total_cost = 0.0;
    double total_profit = 0.0;
    double margin_percent = 0.0;
    int current_stock = 0;
};

struct SaleRecord {
    int id = 0;
    std::string date_time;
    int total_items = 0;
    double total_revenue = 0.0;
    double total_cost = 0.0;
    double total_profit = 0.0;
    double discount = 0.0;
    std::string payment_method = "Efectivo";
    std::string items_summary;
    std::string notes;
};

struct SaleItemDetail {
    std::string sku;
    std::string product_name;
    int quantity = 0;
    double unit_price = 0.0;
    double subtotal = 0.0;
    double profit = 0.0;
};

// ────────────────────────── Business Dashboard ──────────────────────────

struct BusinessMetrics {
    // Sales KPIs
    int total_units_sold = 0;
    int total_sales_count = 0;
    double total_revenue = 0.0;
    double total_cost = 0.0;
    double net_profit = 0.0;
    double average_margin = 0.0;
    double average_ticket = 0.0;

    // Top product
    std::string top_product_name;
    int top_product_units = 0;
    double top_product_revenue = 0.0;

    // Inventory
    int total_products_in_catalog = 0;
    int low_stock_count = 0;
    int out_of_stock_count = 0;
    double inventory_cost_value = 0.0;
    double inventory_sale_value = 0.0;
    double inventory_potential_profit = 0.0;
};

// ────────────────────────── Formatting Helpers ──────────────────────────

inline std::string formatCurrency(double amount) {
    std::ostringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << amount;
    return ss.str();
}

inline std::string formatPercent(double val) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << val << "%";
    return ss.str();
}

inline std::string formatInt(int val) {
    // Add thousands separator
    std::string s = std::to_string(val);
    int insertPos = static_cast<int>(s.length()) - 3;
    while (insertPos > 0) {
        s.insert(insertPos, ",");
        insertPos -= 3;
    }
    return s;
}

#endif // MODELS_HPP
