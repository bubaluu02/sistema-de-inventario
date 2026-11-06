#include "csv_exporter.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

static std::string escapeCsv(const std::string& str) {
    bool needQuotes = false;
    if (str.find(',') != std::string::npos || str.find('"') != std::string::npos || str.find('\n') != std::string::npos) {
        needQuotes = true;
    }
    std::string res;
    for (char c : str) {
        if (c == '"') res += "\"\"";
        else res += c;
    }
    if (needQuotes) {
        return "\"" + res + "\"";
    }
    return res;
}

bool CsvExporter::exportFinancialSummary(const std::string& filepath,
                                         const BusinessMetrics& metrics,
                                         const std::vector<ProductSalesStat>& stats) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    // UTF-8 BOM for Excel
    file << "\xEF\xBB\xBF";

    file << "REPORTE FINANCIERO Y CONTROL DE VENTAS - BARBERIA\n";
    file << "Métricas Globales\n";
    file << "Unidades Vendidas," << metrics.total_units_sold << "\n";
    file << "Ingresos Totales,$" << std::fixed << std::setprecision(2) << metrics.total_revenue << "\n";
    file << "Costo Total de Ventas,$" << std::fixed << std::setprecision(2) << metrics.total_cost << "\n";
    file << "Ganancia Neta,$" << std::fixed << std::setprecision(2) << metrics.net_profit << "\n";
    file << "Margen Promedio," << std::fixed << std::setprecision(1) << metrics.average_margin << "%\n";
    file << "\n";

    file << "DESGLOSE POR PRODUCTO (TIPO EXCEL)\n";
    file << "ID,Código SKU,Producto,Categoría,Unidades Vendidas,Ingresos ($),Costo ($),Ganancia Neta ($),Margen (%),Stock Actual\n";

    for (const auto& s : stats) {
        file << s.product_id << ","
             << escapeCsv(s.sku) << ","
             << escapeCsv(s.name) << ","
             << escapeCsv(s.category) << ","
             << s.units_sold << ","
             << std::fixed << std::setprecision(2) << s.total_revenue << ","
             << std::fixed << std::setprecision(2) << s.total_cost << ","
             << std::fixed << std::setprecision(2) << s.total_profit << ","
             << std::fixed << std::setprecision(1) << s.margin_percent << "%,"
             << s.current_stock << "\n";
    }

    file.close();
    return true;
}

bool CsvExporter::exportInventory(const std::string& filepath,
                                 const std::vector<Product>& products) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "\xEF\xBB\xBF";
    file << "CATÁLOGO E INVENTARIO DE PRODUCTOS DE BARBERÍA\n";
    file << "ID,Código SKU,Nombre del Producto,Categoría,Costo Compra ($),Precio Venta ($),Ganancia Unitaria ($),Margen (%),Stock Actual,Valor en Inventario ($)\n";

    double totalInventoryCost = 0.0;
    double totalInventoryPotentialSales = 0.0;

    for (const auto& p : products) {
        double valCost = p.stock * p.cost_price;
        double valSale = p.stock * p.sale_price;
        totalInventoryCost += valCost;
        totalInventoryPotentialSales += valSale;

        file << p.id << ","
             << escapeCsv(p.sku) << ","
             << escapeCsv(p.name) << ","
             << escapeCsv(p.category) << ","
             << std::fixed << std::setprecision(2) << p.cost_price << ","
             << std::fixed << std::setprecision(2) << p.sale_price << ","
             << std::fixed << std::setprecision(2) << p.unit_profit() << ","
             << std::fixed << std::setprecision(1) << p.margin_percent() << "%,"
             << p.stock << ","
             << std::fixed << std::setprecision(2) << valCost << "\n";
    }

    file << "\n";
    file << ",,,,,,,,TOTAL VALOR INVENTARIO (COSTO),$" << std::fixed << std::setprecision(2) << totalInventoryCost << "\n";
    file << ",,,,,,,,TOTAL VALOR INVENTARIO (VENTA),$" << std::fixed << std::setprecision(2) << totalInventoryPotentialSales << "\n";
    file.close();
    return true;
}

bool CsvExporter::exportSalesHistory(const std::string& filepath,
                                   const std::vector<SaleRecord>& history) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "\xEF\xBB\xBF";
    file << "HISTORIAL DE VENTAS Y TICKETS DE BARBERÍA\n";
    file << "ID Ticket,Fecha y Hora,Cant. Artículos,Detalle de Productos,Total Cobrado ($),Costo ($),Ganancia Neta ($),Notas\n";

    for (const auto& r : history) {
        file << r.id << ","
             << escapeCsv(r.date_time) << ","
             << r.total_items << ","
             << escapeCsv(r.items_summary) << ","
             << std::fixed << std::setprecision(2) << r.total_revenue << ","
             << std::fixed << std::setprecision(2) << r.total_cost << ","
             << std::fixed << std::setprecision(2) << r.total_profit << ","
             << escapeCsv(r.notes) << "\n";
    }

    file.close();
    return true;
}
