#ifndef CSV_EXPORTER_HPP
#define CSV_EXPORTER_HPP

#include "models.hpp"
#include <vector>
#include <string>

class CsvExporter {
public:
    static bool exportFinancialSummary(const std::string& filepath, 
                                       const BusinessMetrics& metrics, 
                                       const std::vector<ProductSalesStat>& stats);

    static bool exportInventory(const std::string& filepath, 
                                const std::vector<Product>& products);

    static bool exportSalesHistory(const std::string& filepath, 
                                  const std::vector<SaleRecord>& history);
};

#endif // CSV_EXPORTER_HPP
