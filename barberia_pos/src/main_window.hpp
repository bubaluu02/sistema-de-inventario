#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include "models.hpp"
#include "database.hpp"
#include "receipt_printer.hpp"
#include <gtkmm.h>
#include <vector>
#include <memory>

class MainWindow : public Gtk::Window {
public:
    MainWindow();
    virtual ~MainWindow() = default;

private:
    void setupUI();
    void setupRibbon();
    void setupTabSummary();
    void setupTabInventory();
    void setupTabSalesPOS();
    void setupTabHistory();
    void setupStatusBar();

    // Refresh data methods
    void refreshAll();
    void refreshSummary();
    void refreshInventory();
    void refreshPOSCatalog();
    void refreshHistory();
    void refreshKPIs();

    // Event handlers - Inventory
    void onAddProductClicked();
    void onEditProductClicked();
    void onDeleteProductClicked();
    void onInventoryFilterChanged();

    // Event handlers - POS
    void onPosProductChanged();
    void onPosAddToCartClicked();
    void onPosRemoveFromCartClicked();
    void onPosClearCartClicked();
    void onPosCashChanged();
    void onPosDiscountChanged();
    void onPosCheckoutClicked();
    void onExchangeRateChanged();

    // Event handlers - Exports & Time Filter & Receipts
    void onTimeFilterChanged();
    void onExportSummaryClicked();
    void onExportInventoryClicked();
    void onExportHistoryClicked();
    void onOpenReceiptFolderClicked();
    void onPrintSelectedHistoryTicketClicked();
    void onDeleteHistorySaleClicked();

    // Conversion helper
    double getExchangeRate() const;
    std::string formatHNL(double lps) const;
    std::string formatUSD(double lps) const;
    std::string formatDual(double lps) const;

    // TreeView Model Columns
    // 1. Summary Columns
    class SummaryColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        SummaryColumns() {
            add(col_id); add(col_sku); add(col_name); add(col_brand); add(col_category);
            add(col_units); add(col_revenue); add(col_cost); add(col_profit);
            add(col_margin); add(col_stock);
        }
        Gtk::TreeModelColumn<int> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_sku;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_brand;
        Gtk::TreeModelColumn<Glib::ustring> col_category;
        Gtk::TreeModelColumn<int> col_units;
        Gtk::TreeModelColumn<Glib::ustring> col_revenue;
        Gtk::TreeModelColumn<Glib::ustring> col_cost;
        Gtk::TreeModelColumn<Glib::ustring> col_profit;
        Gtk::TreeModelColumn<Glib::ustring> col_margin;
        Gtk::TreeModelColumn<int> col_stock;
    } m_summaryCols;
    Glib::RefPtr<Gtk::ListStore> m_summaryStore;
    Gtk::TreeView m_tvSummary;

    // 2. Inventory Columns
    class InventoryColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        InventoryColumns() {
            add(col_id); add(col_sku); add(col_name); add(col_brand); add(col_category);
            add(col_cost); add(col_price); add(col_unit_profit);
            add(col_margin); add(col_stock); add(col_status);
        }
        Gtk::TreeModelColumn<int> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_sku;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<Glib::ustring> col_brand;
        Gtk::TreeModelColumn<Glib::ustring> col_category;
        Gtk::TreeModelColumn<Glib::ustring> col_cost;
        Gtk::TreeModelColumn<Glib::ustring> col_price;
        Gtk::TreeModelColumn<Glib::ustring> col_unit_profit;
        Gtk::TreeModelColumn<Glib::ustring> col_margin;
        Gtk::TreeModelColumn<int> col_stock;
        Gtk::TreeModelColumn<Glib::ustring> col_status;
    } m_invCols;
    Glib::RefPtr<Gtk::ListStore> m_invStore;
    Gtk::TreeView m_tvInventory;

    // 3. POS Cart Columns
    class CartColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        CartColumns() {
            add(col_id); add(col_sku); add(col_name);
            add(col_qty); add(col_unit_price); add(col_subtotal); add(col_profit);
        }
        Gtk::TreeModelColumn<int> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_sku;
        Gtk::TreeModelColumn<Glib::ustring> col_name;
        Gtk::TreeModelColumn<int> col_qty;
        Gtk::TreeModelColumn<Glib::ustring> col_unit_price;
        Gtk::TreeModelColumn<Glib::ustring> col_subtotal;
        Gtk::TreeModelColumn<Glib::ustring> col_profit;
    } m_cartCols;
    Glib::RefPtr<Gtk::ListStore> m_cartStore;
    Gtk::TreeView m_tvCart;

    // 4. Sales History Columns
    class HistoryColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        HistoryColumns() {
            add(col_id); add(col_date); add(col_items_count);
            add(col_details); add(col_method); add(col_revenue); add(col_cost); add(col_profit);
        }
        Gtk::TreeModelColumn<int> col_id;
        Gtk::TreeModelColumn<Glib::ustring> col_date;
        Gtk::TreeModelColumn<int> col_items_count;
        Gtk::TreeModelColumn<Glib::ustring> col_details;
        Gtk::TreeModelColumn<Glib::ustring> col_method;
        Gtk::TreeModelColumn<Glib::ustring> col_revenue;
        Gtk::TreeModelColumn<Glib::ustring> col_cost;
        Gtk::TreeModelColumn<Glib::ustring> col_profit;
    } m_histCols;
    Glib::RefPtr<Gtk::ListStore> m_histStore;
    Gtk::TreeView m_tvHistory;

    // Main layout widgets
    Gtk::Box m_rootBox{Gtk::Orientation::VERTICAL, 0};
    Gtk::Notebook m_notebook;

    // Time filter combo
    Gtk::ComboBoxText m_comboTimeFilter;

    // Exchange Rate Entry (USD -> HNL)
    Gtk::SpinButton m_spinExchangeRate;

    // KPI Card Labels
    Gtk::Label m_lblKpiUnits;
    Gtk::Label m_lblKpiRevenue;
    Gtk::Label m_lblKpiCost;
    Gtk::Label m_lblKpiProfit;
    Gtk::Label m_lblKpiMargin;
    Gtk::Label m_lblKpiTicket;
    Gtk::Label m_lblKpiTop;

    // Search and filter entries
    Gtk::SearchEntry m_searchSummary;
    Gtk::SearchEntry m_searchInventory;

    // Inventory Tab Buttons
    Gtk::Button m_btnAddProduct{"➕ Agregar Producto"};
    Gtk::Button m_btnEditProduct{"✏️ Editar Producto"};
    Gtk::Button m_btnDeleteProduct{"🗑️ Quitar Producto"};
    Gtk::Label m_lblInventoryStats;

    // POS Tab controls
    Gtk::ComboBoxText m_posProductCombo;
    Gtk::SpinButton m_posSpinQty;
    Gtk::Label m_posLblStockInfo;
    Gtk::Label m_posLblItemPrice;
    Gtk::Button m_posBtnAddCart{"➕ Agregar al Ticket"};
    Gtk::Button m_posBtnRemoveCart{"➖ Quitar Selección"};
    Gtk::Button m_posBtnClearCart{"🗑️ Vaciar Ticket"};

    // Payment and discount controls
    Gtk::ComboBoxText m_posComboPayment;
    Gtk::SpinButton m_posSpinDiscount;

    Gtk::Label m_posLblTotalPay;
    Gtk::Label m_posLblTotalUSD;
    Gtk::Label m_posLblEstimatedProfit;
    Gtk::Entry m_posEntryCash;
    Gtk::Label m_posLblChange;
    Gtk::CheckButton m_chkAutoPrintReceipt{"Imprimir / Guardar recibo automáticamente"};
    Gtk::Button m_posBtnCheckout{"✅ Cobrar y Registrar Venta"};

    // POS Internal state
    std::vector<CartItem> m_cartItems;
    std::vector<Product> m_posProductList;

    // Status bar
    Gtk::Label m_lblStatusDb;
    Gtk::Label m_lblStatusCatalog;
    Gtk::Label m_lblStatusAction;
};

#endif // MAIN_WINDOW_HPP
