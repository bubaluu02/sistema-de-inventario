#include "main_window.hpp"
#include "product_dialog.hpp"
#include "csv_exporter.hpp"
#include "receipt_printer.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>

MainWindow::MainWindow() {
    set_title("BarberShop POS • Control Financiero e Inventario (Lempiras & Dólares)");
    set_default_size(960, 600);

    setupUI();
    refreshAll();
}

double MainWindow::getExchangeRate() const {
    double rate = m_spinExchangeRate.get_value();
    return rate > 0.0 ? rate : 24.75;
}

std::string MainWindow::formatHNL(double lps) const {
    std::ostringstream ss;
    ss << "L " << std::fixed << std::setprecision(2) << lps;
    return ss.str();
}

std::string MainWindow::formatUSD(double lps) const {
    double usd = lps / getExchangeRate();
    std::ostringstream ss;
    ss << "$" << std::fixed << std::setprecision(2) << usd;
    return ss.str();
}

std::string MainWindow::formatDual(double lps) const {
    return formatHNL(lps) + " (" + formatUSD(lps) + ")";
}

void MainWindow::setupUI() {
    setupRibbon();

    // Setup Tabs (Excel Sheets)
    setupTabSummary();
    setupTabInventory();
    setupTabSalesPOS();
    setupTabHistory();

    m_rootBox.append(m_notebook);

    setupStatusBar();

    set_child(m_rootBox);
}

void MainWindow::setupRibbon() {
    auto ribbon = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    ribbon->add_css_class("excel-ribbon-bar");

    // Title & Icon
    auto titleBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 2);
    auto lblTitle = Gtk::make_managed<Gtk::Label>("💈 BarberShop Pro — Sistema de Ventas y Finanzas");
    lblTitle->add_css_class("excel-title");
    lblTitle->set_halign(Gtk::Align::START);

    auto lblSub = Gtk::make_managed<Gtk::Label>("Moneda: Lempiras (HNL) • Conversor USD • Impresión de Recibos");
    lblSub->add_css_class("excel-subtitle");
    lblSub->set_halign(Gtk::Align::START);

    titleBox->append(*lblTitle);
    titleBox->append(*lblSub);
    ribbon->append(*titleBox);

    // Spacer
    auto spacer = Gtk::make_managed<Gtk::Box>();
    spacer->set_hexpand(true);
    ribbon->append(*spacer);

    // Exchange Rate Box in Ribbon
    auto fxBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 6);
    auto lblFx = Gtk::make_managed<Gtk::Label>("Tipo Cambio USD/HNL:");
    m_spinExchangeRate.set_range(1.0, 100.0);
    m_spinExchangeRate.set_increments(0.05, 1.0);
    m_spinExchangeRate.set_digits(2);
    m_spinExchangeRate.set_value(24.75);
    m_spinExchangeRate.signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::onExchangeRateChanged));
    fxBox->append(*lblFx);
    fxBox->append(m_spinExchangeRate);
    ribbon->append(*fxBox);

    // Time filter combo
    m_comboTimeFilter.append("all", "📅 Todo el Tiempo");
    m_comboTimeFilter.append("today", "📅 Hoy");
    m_comboTimeFilter.append("week", "📅 Esta Semana");
    m_comboTimeFilter.append("month", "📅 Este Mes");
    m_comboTimeFilter.set_active_id("all");
    m_comboTimeFilter.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::onTimeFilterChanged));
    ribbon->append(m_comboTimeFilter);

    // Quick Ribbon Buttons
    auto btnQuickAdd = Gtk::make_managed<Gtk::Button>("➕ Nuevo Producto");
    btnQuickAdd->add_css_class("ribbon-btn");
    btnQuickAdd->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onAddProductClicked));

    auto btnQuickPOS = Gtk::make_managed<Gtk::Button>("🛒 Cobrar Venta");
    btnQuickPOS->add_css_class("ribbon-btn");
    btnQuickPOS->signal_clicked().connect([this]() {
        m_notebook.set_current_page(2);
    });

    auto btnExportAll = Gtk::make_managed<Gtk::Button>("📥 Exportar a Excel");
    btnExportAll->add_css_class("ribbon-btn");
    btnExportAll->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onExportSummaryClicked));

    auto btnOpenReceipts = Gtk::make_managed<Gtk::Button>("🧾 Carpeta Recibos");
    btnOpenReceipts->add_css_class("ribbon-btn");
    btnOpenReceipts->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenReceiptFolderClicked));

    auto btnRefresh = Gtk::make_managed<Gtk::Button>("🔄 Actualizar");
    btnRefresh->add_css_class("ribbon-btn");
    btnRefresh->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::refreshAll));

    ribbon->append(*btnQuickAdd);
    ribbon->append(*btnQuickPOS);
    ribbon->append(*btnOpenReceipts);
    ribbon->append(*btnExportAll);
    ribbon->append(*btnRefresh);

    m_rootBox.append(*ribbon);
}

void MainWindow::setupTabSummary() {
    auto tabBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    tabBox->set_margin(12);

    // 1. KPI Cards Panel (7 cards)
    auto kpiGrid = Gtk::make_managed<Gtk::Grid>();
    kpiGrid->set_column_spacing(10);
    kpiGrid->set_row_spacing(8);
    kpiGrid->set_hexpand(true);

    auto createKpiCard = [](const std::string& title, Gtk::Label& valLabel, const std::string& valClass, const std::string& subtext) {
        auto frame = Gtk::make_managed<Gtk::Frame>();
        frame->add_css_class("kpi-card");
        frame->set_hexpand(true);

        auto box = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 3);
        auto lTitle = Gtk::make_managed<Gtk::Label>(title);
        lTitle->add_css_class("kpi-title");
        lTitle->set_halign(Gtk::Align::START);

        valLabel.add_css_class(valClass);
        valLabel.set_halign(Gtk::Align::START);

        auto lSub = Gtk::make_managed<Gtk::Label>(subtext);
        lSub->add_css_class("kpi-subtext");
        lSub->set_halign(Gtk::Align::START);

        box->append(*lTitle);
        box->append(valLabel);
        box->append(*lSub);
        frame->set_child(*box);
        return frame;
    };

    kpiGrid->attach(*createKpiCard("📦 Unidades Vendidas", m_lblKpiUnits, "kpi-value-blue", "Volumen en el período"), 0, 0);
    kpiGrid->attach(*createKpiCard("💰 Ingresos Totales", m_lblKpiRevenue, "kpi-value-blue", "Ventas brutas acumuladas"), 1, 0);
    kpiGrid->attach(*createKpiCard("📉 Costo de Ventas", m_lblKpiCost, "kpi-value-amber", "Costo de mercancía vendida"), 2, 0);
    kpiGrid->attach(*createKpiCard("🚀 Ganancia Neta Real", m_lblKpiProfit, "kpi-value-green", "Utilidad limpia en caja"), 3, 0);
    kpiGrid->attach(*createKpiCard("📊 Margen Promedio", m_lblKpiMargin, "kpi-value-green", "Rentabilidad sobre ventas"), 0, 1);
    kpiGrid->attach(*createKpiCard("🧾 Ticket Promedio", m_lblKpiTicket, "kpi-value-blue", "Promedio gastado por cliente"), 1, 1);
    kpiGrid->attach(*createKpiCard("🏆 Producto Estrella", m_lblKpiTop, "kpi-value-purple", "Más vendido del período"), 2, 1, 2, 1);

    tabBox->append(*kpiGrid);

    // 2. Toolbar & Filter
    auto toolBar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    auto lblGridTitle = Gtk::make_managed<Gtk::Label>("📋 Desglose de Ventas y Rentabilidad por Producto (HNL & USD)");
    lblGridTitle->set_halign(Gtk::Align::START);
    lblGridTitle->add_css_class("dialog-title");

    m_searchSummary.set_placeholder_text("🔍 Filtrar por nombre, marca o SKU...");
    m_searchSummary.set_width_chars(28);
    m_searchSummary.signal_search_changed().connect(sigc::mem_fun(*this, &MainWindow::refreshSummary));

    auto btnExpSummary = Gtk::make_managed<Gtk::Button>("📥 Exportar a Excel (.csv)");
    btnExpSummary->add_css_class("btn-action-secondary");
    btnExpSummary->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onExportSummaryClicked));

    toolBar->append(*lblGridTitle);
    auto sp = Gtk::make_managed<Gtk::Box>();
    sp->set_hexpand(true);
    toolBar->append(*sp);
    toolBar->append(m_searchSummary);
    toolBar->append(*btnExpSummary);

    tabBox->append(*toolBar);

    // 3. Excel Grid (TreeView)
    m_summaryStore = Gtk::ListStore::create(m_summaryCols);
    m_tvSummary.set_model(m_summaryStore);
    m_tvSummary.set_grid_lines(Gtk::TreeView::GridLines::BOTH);
    m_tvSummary.set_headers_visible(true);

    m_tvSummary.append_column("ID", m_summaryCols.col_id);
    m_tvSummary.append_column("Código", m_summaryCols.col_sku);
    m_tvSummary.append_column("Producto", m_summaryCols.col_name);
    m_tvSummary.append_column("Marca", m_summaryCols.col_brand);
    m_tvSummary.append_column("Categoría", m_summaryCols.col_category);

    auto colU = Gtk::make_managed<Gtk::TreeViewColumn>("Uds. Vendidas", m_summaryCols.col_units);
    colU->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colU->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colU);

    auto colR = Gtk::make_managed<Gtk::TreeViewColumn>("Ingresos (HNL / USD)", m_summaryCols.col_revenue);
    colR->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colR->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colR);

    auto colC = Gtk::make_managed<Gtk::TreeViewColumn>("Costo (HNL)", m_summaryCols.col_cost);
    colC->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colC->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colC);

    auto colP = Gtk::make_managed<Gtk::TreeViewColumn>("Ganancia Neta", m_summaryCols.col_profit);
    colP->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colP->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colP);

    auto colM = Gtk::make_managed<Gtk::TreeViewColumn>("Margen %", m_summaryCols.col_margin);
    colM->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colM->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colM);

    auto colS = Gtk::make_managed<Gtk::TreeViewColumn>("Stock Actual", m_summaryCols.col_stock);
    colS->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colS->get_first_cell())) c->property_xalign() = 1.0;
    m_tvSummary.append_column(*colS);

    for (guint i = 0; i < m_tvSummary.get_columns().size(); ++i) {
        auto col = m_tvSummary.get_column(i);
        if (col) {
            col->set_resizable(true);
            col->set_reorderable(true);
        }
    }

    auto scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_child(m_tvSummary);
    scroll->set_vexpand(true);
    scroll->set_hexpand(true);
    tabBox->append(*scroll);

    m_notebook.append_page(*tabBox, "📊 Resumen y Ganancias");
}

void MainWindow::setupTabInventory() {
    auto tabBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    tabBox->set_margin(12);

    // 1. Toolbar with Add, Edit, Delete Buttons
    auto toolBar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);

    m_btnAddProduct.add_css_class("btn-action-primary");
    m_btnAddProduct.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onAddProductClicked));

    m_btnEditProduct.add_css_class("btn-action-secondary");
    m_btnEditProduct.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onEditProductClicked));

    m_btnDeleteProduct.add_css_class("btn-action-danger");
    m_btnDeleteProduct.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDeleteProductClicked));

    auto btnExpInv = Gtk::make_managed<Gtk::Button>("📥 Exportar Inventario");
    btnExpInv->add_css_class("btn-action-secondary");
    btnExpInv->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onExportInventoryClicked));

    m_searchInventory.set_placeholder_text("🔍 Buscar por nombre, marca o SKU...");
    m_searchInventory.set_width_chars(28);
    m_searchInventory.signal_search_changed().connect(sigc::mem_fun(*this, &MainWindow::onInventoryFilterChanged));

    toolBar->append(m_btnAddProduct);
    toolBar->append(m_btnEditProduct);
    toolBar->append(m_btnDeleteProduct);
    toolBar->append(*btnExpInv);

    auto sp = Gtk::make_managed<Gtk::Box>();
    sp->set_hexpand(true);
    toolBar->append(*sp);
    toolBar->append(m_searchInventory);

    tabBox->append(*toolBar);

    // 2. Info / helper banner
    auto banner = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    m_lblInventoryStats.set_halign(Gtk::Align::START);
    m_lblInventoryStats.add_css_class("kpi-subtext");
    banner->append(m_lblInventoryStats);
    tabBox->append(*banner);

    // 3. Excel Grid (TreeView)
    m_invStore = Gtk::ListStore::create(m_invCols);
    m_tvInventory.set_model(m_invStore);
    m_tvInventory.set_grid_lines(Gtk::TreeView::GridLines::BOTH);
    m_tvInventory.set_headers_visible(true);

    m_tvInventory.append_column("ID", m_invCols.col_id);
    m_tvInventory.append_column("Código", m_invCols.col_sku);
    m_tvInventory.append_column("Producto", m_invCols.col_name);
    m_tvInventory.append_column("Marca", m_invCols.col_brand);
    m_tvInventory.append_column("Categoría", m_invCols.col_category);

    auto colC = Gtk::make_managed<Gtk::TreeViewColumn>("Costo (HNL)", m_invCols.col_cost);
    colC->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colC->get_first_cell())) c->property_xalign() = 1.0;
    m_tvInventory.append_column(*colC);

    auto colP = Gtk::make_managed<Gtk::TreeViewColumn>("Precio Venta (HNL / USD)", m_invCols.col_price);
    colP->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colP->get_first_cell())) c->property_xalign() = 1.0;
    m_tvInventory.append_column(*colP);

    auto colU = Gtk::make_managed<Gtk::TreeViewColumn>("Ganancia/U (HNL)", m_invCols.col_unit_profit);
    colU->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colU->get_first_cell())) c->property_xalign() = 1.0;
    m_tvInventory.append_column(*colU);

    auto colM = Gtk::make_managed<Gtk::TreeViewColumn>("Margen %", m_invCols.col_margin);
    colM->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colM->get_first_cell())) c->property_xalign() = 1.0;
    m_tvInventory.append_column(*colM);

    auto colS = Gtk::make_managed<Gtk::TreeViewColumn>("Stock", m_invCols.col_stock);
    colS->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colS->get_first_cell())) c->property_xalign() = 1.0;
    m_tvInventory.append_column(*colS);

    m_tvInventory.append_column("Estado", m_invCols.col_status);

    for (guint i = 0; i < m_tvInventory.get_columns().size(); ++i) {
        auto col = m_tvInventory.get_column(i);
        if (col) {
            col->set_resizable(true);
            col->set_reorderable(true);
        }
    }

    // Double click to edit product
    m_tvInventory.signal_row_activated().connect([this](const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*) {
        onEditProductClicked();
    });

    auto scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_child(m_tvInventory);
    scroll->set_vexpand(true);
    scroll->set_hexpand(true);
    tabBox->append(*scroll);

    m_notebook.append_page(*tabBox, "📦 Inventario y Productos");
}

void MainWindow::setupTabSalesPOS() {
    auto tabBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    tabBox->set_margin(14);

    // Left Panel: Product Selector & Add to Cart
    auto leftBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    leftBox->set_size_request(320, -1);

    auto lblPosTitle = Gtk::make_managed<Gtk::Label>("🛒 Registrar Venta de Mostrador");
    lblPosTitle->add_css_class("dialog-title");
    lblPosTitle->set_halign(Gtk::Align::START);
    leftBox->append(*lblPosTitle);

    auto grid = Gtk::make_managed<Gtk::Grid>();
    grid->set_row_spacing(8);
    grid->set_column_spacing(10);

    auto lblPick = Gtk::make_managed<Gtk::Label>("Producto:");
    lblPick->set_halign(Gtk::Align::START);
    m_posProductCombo.set_hexpand(true);
    m_posProductCombo.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::onPosProductChanged));
    grid->attach(*lblPick, 0, 0);
    grid->attach(m_posProductCombo, 1, 0);

    auto lblQty = Gtk::make_managed<Gtk::Label>("Cantidad:");
    lblQty->set_halign(Gtk::Align::START);
    m_posSpinQty.set_range(1, 100);
    m_posSpinQty.set_increments(1, 5);
    m_posSpinQty.set_value(1);
    grid->attach(*lblQty, 0, 1);
    grid->attach(m_posSpinQty, 1, 1);

    auto lblP = Gtk::make_managed<Gtk::Label>("Precio:");
    lblP->set_halign(Gtk::Align::START);
    m_posLblItemPrice.set_text("L 0.00 ($0.00)");
    m_posLblItemPrice.add_css_class("preview-metric-profit");
    m_posLblItemPrice.set_halign(Gtk::Align::START);
    grid->attach(*lblP, 0, 2);
    grid->attach(m_posLblItemPrice, 1, 2);

    auto lblS = Gtk::make_managed<Gtk::Label>("Stock:");
    lblS->set_halign(Gtk::Align::START);
    m_posLblStockInfo.set_text("0 unidades");
    m_posLblStockInfo.set_halign(Gtk::Align::START);
    grid->attach(*lblS, 0, 3);
    grid->attach(m_posLblStockInfo, 1, 3);

    leftBox->append(*grid);

    m_posBtnAddCart.add_css_class("btn-action-primary");
    m_posBtnAddCart.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onPosAddToCartClicked));
    leftBox->append(m_posBtnAddCart);

    // Financial Checkout Card
    auto totalCard = Gtk::make_managed<Gtk::Frame>();
    totalCard->add_css_class("pos-total-card");
    auto tcBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 6);

    auto lblTotTitle = Gtk::make_managed<Gtk::Label>("TOTAL A PAGAR (HNL)");
    lblTotTitle->add_css_class("kpi-title");
    lblTotTitle->set_halign(Gtk::Align::START);

    m_posLblTotalPay.set_text("L 0.00");
    m_posLblTotalPay.add_css_class("pos-total-amount");
    m_posLblTotalPay.set_halign(Gtk::Align::START);

    m_posLblTotalUSD.set_text("Equivalente en Dólares: $0.00 USD");
    m_posLblTotalUSD.add_css_class("preview-metric-margin");
    m_posLblTotalUSD.set_halign(Gtk::Align::START);

    m_posLblEstimatedProfit.set_text("Ganancia neta estimada: +L 0.00");
    m_posLblEstimatedProfit.add_css_class("pos-profit-badge");
    m_posLblEstimatedProfit.set_halign(Gtk::Align::START);

    tcBox->append(*lblTotTitle);
    tcBox->append(m_posLblTotalPay);
    tcBox->append(m_posLblTotalUSD);
    tcBox->append(m_posLblEstimatedProfit);

    // Payment method & Discount
    auto payGrid = Gtk::make_managed<Gtk::Grid>();
    payGrid->set_row_spacing(6);
    payGrid->set_column_spacing(8);

    auto lblPay = Gtk::make_managed<Gtk::Label>("Método de Pago:");
    m_posComboPayment.append("Efectivo", "💵 Efectivo (HNL/USD)");
    m_posComboPayment.append("Tarjeta", "💳 Tarjeta Débito/Crédito");
    m_posComboPayment.append("Transferencia", "📱 Transferencia Bancaria");
    m_posComboPayment.set_active_id("Efectivo");
    payGrid->attach(*lblPay, 0, 0);
    payGrid->attach(m_posComboPayment, 1, 0);

    auto lblDisc = Gtk::make_managed<Gtk::Label>("Descuento (L):");
    m_posSpinDiscount.set_range(0.0, 5000.0);
    m_posSpinDiscount.set_increments(5.0, 50.0);
    m_posSpinDiscount.set_digits(2);
    m_posSpinDiscount.set_value(0.0);
    m_posSpinDiscount.signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::onPosDiscountChanged));
    payGrid->attach(*lblDisc, 0, 1);
    payGrid->attach(m_posSpinDiscount, 1, 1);

    auto lblCash = Gtk::make_managed<Gtk::Label>("Efectivo Recibido (L):");
    m_posEntryCash.set_placeholder_text("L 0.00");
    m_posEntryCash.signal_changed().connect(sigc::mem_fun(*this, &MainWindow::onPosCashChanged));
    payGrid->attach(*lblCash, 0, 2);
    payGrid->attach(m_posEntryCash, 1, 2);

    tcBox->append(*payGrid);

    m_posLblChange.set_text("Cambio a devolver: L 0.00");
    m_posLblChange.set_halign(Gtk::Align::START);
    m_posLblChange.add_css_class("dialog-subtitle");
    tcBox->append(m_posLblChange);

    // Auto print receipt check
    m_chkAutoPrintReceipt.set_active(true);
    tcBox->append(m_chkAutoPrintReceipt);

    totalCard->set_child(*tcBox);
    leftBox->append(*totalCard);

    m_posBtnCheckout.add_css_class("btn-action-primary");
    m_posBtnCheckout.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onPosCheckoutClicked));
    leftBox->append(m_posBtnCheckout);

    tabBox->append(*leftBox);

    // Right Panel: Cart / Ticket Table
    auto rightBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 8);
    rightBox->set_hexpand(true);

    auto cartHeaderBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    auto lblCartTitle = Gtk::make_managed<Gtk::Label>("🧾 Artículos en el Ticket Actual");
    lblCartTitle->add_css_class("dialog-title");
    lblCartTitle->set_halign(Gtk::Align::START);

    m_posBtnRemoveCart.add_css_class("btn-action-secondary");
    m_posBtnRemoveCart.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onPosRemoveFromCartClicked));

    m_posBtnClearCart.add_css_class("btn-action-danger");
    m_posBtnClearCart.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onPosClearCartClicked));

    cartHeaderBox->append(*lblCartTitle);
    auto sp = Gtk::make_managed<Gtk::Box>();
    sp->set_hexpand(true);
    cartHeaderBox->append(*sp);
    cartHeaderBox->append(m_posBtnRemoveCart);
    cartHeaderBox->append(m_posBtnClearCart);
    rightBox->append(*cartHeaderBox);

    m_cartStore = Gtk::ListStore::create(m_cartCols);
    m_tvCart.set_model(m_cartStore);
    m_tvCart.set_grid_lines(Gtk::TreeView::GridLines::BOTH);

    m_tvCart.append_column("SKU", m_cartCols.col_sku);
    m_tvCart.append_column("Producto", m_cartCols.col_name);
    m_tvCart.append_column("Cantidad", m_cartCols.col_qty);

    auto colUP = Gtk::make_managed<Gtk::TreeViewColumn>("P. Unitario", m_cartCols.col_unit_price);
    colUP->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colUP->get_first_cell())) c->property_xalign() = 1.0;
    m_tvCart.append_column(*colUP);

    auto colSub = Gtk::make_managed<Gtk::TreeViewColumn>("Subtotal", m_cartCols.col_subtotal);
    colSub->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colSub->get_first_cell())) c->property_xalign() = 1.0;
    m_tvCart.append_column(*colSub);

    auto colPrf = Gtk::make_managed<Gtk::TreeViewColumn>("Ganancia Est.", m_cartCols.col_profit);
    colPrf->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colPrf->get_first_cell())) c->property_xalign() = 1.0;
    m_tvCart.append_column(*colPrf);

    auto cartScroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    cartScroll->set_child(m_tvCart);
    cartScroll->set_vexpand(true);
    rightBox->append(*cartScroll);

    tabBox->append(*rightBox);

    m_notebook.append_page(*tabBox, "🛒 Registrar Venta");
}

void MainWindow::setupTabHistory() {
    auto tabBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 10);
    tabBox->set_margin(12);

    auto toolBar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 10);
    auto lblHistTitle = Gtk::make_managed<Gtk::Label>("📜 Historial de Ventas, Facturación y Tickets");
    lblHistTitle->add_css_class("dialog-title");
    lblHistTitle->set_halign(Gtk::Align::START);

    auto btnPrintTicket = Gtk::make_managed<Gtk::Button>("🖨️ Imprimir Ticket Seleccionado");
    btnPrintTicket->add_css_class("btn-action-primary");
    btnPrintTicket->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onPrintSelectedHistoryTicketClicked));

    auto btnOpenFolder = Gtk::make_managed<Gtk::Button>("📂 Abrir Carpeta de Tickets");
    btnOpenFolder->add_css_class("btn-action-secondary");
    btnOpenFolder->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenReceiptFolderClicked));

    auto btnDeleteSale = Gtk::make_managed<Gtk::Button>("🗑️ Eliminar Venta");
    btnDeleteSale->add_css_class("btn-action-danger");
    btnDeleteSale->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDeleteHistorySaleClicked));

    auto btnExpHist = Gtk::make_managed<Gtk::Button>("📥 Exportar a Excel");
    btnExpHist->add_css_class("btn-action-secondary");
    btnExpHist->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onExportHistoryClicked));

    toolBar->append(*lblHistTitle);
    auto sp = Gtk::make_managed<Gtk::Box>();
    sp->set_hexpand(true);
    toolBar->append(*sp);
    toolBar->append(*btnPrintTicket);
    toolBar->append(*btnDeleteSale);
    toolBar->append(*btnOpenFolder);
    toolBar->append(*btnExpHist);
    tabBox->append(*toolBar);

    m_histStore = Gtk::ListStore::create(m_histCols);
    m_tvHistory.set_model(m_histStore);
    m_tvHistory.set_grid_lines(Gtk::TreeView::GridLines::BOTH);

    m_tvHistory.append_column("Ticket #", m_histCols.col_id);
    m_tvHistory.append_column("Fecha y Hora", m_histCols.col_date);
    m_tvHistory.append_column("Total Uds.", m_histCols.col_items_count);

    auto colDet = Gtk::make_managed<Gtk::TreeViewColumn>("Detalle de Productos Vendidos", m_histCols.col_details);
    colDet->set_resizable(true);
    colDet->set_min_width(280);
    m_tvHistory.append_column(*colDet);

    m_tvHistory.append_column("Método Pago", m_histCols.col_method);

    auto colR = Gtk::make_managed<Gtk::TreeViewColumn>("Total Cobrado", m_histCols.col_revenue);
    colR->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colR->get_first_cell())) c->property_xalign() = 1.0;
    m_tvHistory.append_column(*colR);

    auto colC = Gtk::make_managed<Gtk::TreeViewColumn>("Costo", m_histCols.col_cost);
    colC->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colC->get_first_cell())) c->property_xalign() = 1.0;
    m_tvHistory.append_column(*colC);

    auto colP = Gtk::make_managed<Gtk::TreeViewColumn>("Ganancia Neta", m_histCols.col_profit);
    colP->set_alignment(1.0);
    if (auto c = dynamic_cast<Gtk::CellRendererText*>(colP->get_first_cell())) c->property_xalign() = 1.0;
    m_tvHistory.append_column(*colP);

    for (guint i = 0; i < m_tvHistory.get_columns().size(); ++i) {
        auto col = m_tvHistory.get_column(i);
        if (col) col->set_resizable(true);
    }

    m_tvHistory.signal_row_activated().connect([this](const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*) {
        onPrintSelectedHistoryTicketClicked();
    });

    auto scroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    scroll->set_child(m_tvHistory);
    scroll->set_vexpand(true);
    scroll->set_hexpand(true);
    tabBox->append(*scroll);

    m_notebook.append_page(*tabBox, "📜 Historial de Ventas");
}

void MainWindow::setupStatusBar() {
    auto bar = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 16);
    bar->add_css_class("excel-status-bar");

    m_lblStatusDb.set_text("🟢 SQLite: " + Database::instance().getDatabasePath());
    m_lblStatusDb.set_halign(Gtk::Align::START);

    auto sp = Gtk::make_managed<Gtk::Box>();
    sp->set_hexpand(true);

    m_lblStatusCatalog.set_text("Catálogo: 0 productos");
    m_lblStatusAction.set_text("Listo para operar");

    bar->append(m_lblStatusDb);
    bar->append(*sp);
    bar->append(m_lblStatusCatalog);
    bar->append(m_lblStatusAction);

    m_rootBox.append(*bar);
}

void MainWindow::refreshAll() {
    refreshKPIs();
    refreshSummary();
    refreshInventory();
    refreshPOSCatalog();
    refreshHistory();
}

static TimeFilter getTimeFilterFromCombo(const std::string& id) {
    if (id == "today") return TimeFilter::TODAY;
    if (id == "week") return TimeFilter::THIS_WEEK;
    if (id == "month") return TimeFilter::THIS_MONTH;
    return TimeFilter::ALL_TIME;
}

void MainWindow::refreshKPIs() {
    auto tf = getTimeFilterFromCombo(m_comboTimeFilter.get_active_id());
    auto m = Database::instance().getMetrics(tf);

    m_lblKpiUnits.set_text(std::to_string(m.total_units_sold) + " uds");
    m_lblKpiRevenue.set_text(formatDual(m.total_revenue));
    m_lblKpiCost.set_text(formatDual(m.total_cost));
    m_lblKpiProfit.set_text(formatDual(m.net_profit));
    m_lblKpiMargin.set_text(formatPercent(m.average_margin));
    m_lblKpiTicket.set_text(formatDual(m.average_ticket));

    if (!m.top_product_name.empty() && m.top_product_name != "N/A") {
        m_lblKpiTop.set_text(m.top_product_name + " (" + std::to_string(m.top_product_units) + " uds)");
    } else {
        m_lblKpiTop.set_text("Ninguno aún");
    }

    m_lblStatusCatalog.set_text("Catálogo: " + std::to_string(m.total_products_in_catalog) + " prods | Stock bajo: " + std::to_string(m.low_stock_count) + " | Inversión: " + formatHNL(m.inventory_cost_value));
}

void MainWindow::refreshSummary() {
    m_summaryStore->clear();
    std::string filter = m_searchSummary.get_text();
    auto tf = getTimeFilterFromCombo(m_comboTimeFilter.get_active_id());
    auto stats = Database::instance().getProductSalesBreakdown(filter, tf);

    for (const auto& s : stats) {
        auto row = *(m_summaryStore->append());
        row[m_summaryCols.col_id] = s.product_id;
        row[m_summaryCols.col_sku] = s.sku;
        row[m_summaryCols.col_name] = s.name;
        row[m_summaryCols.col_brand] = s.brand;
        row[m_summaryCols.col_category] = s.category;
        row[m_summaryCols.col_units] = s.units_sold;
        row[m_summaryCols.col_revenue] = formatDual(s.total_revenue);
        row[m_summaryCols.col_cost] = formatHNL(s.total_cost);
        row[m_summaryCols.col_profit] = formatDual(s.total_profit);
        row[m_summaryCols.col_margin] = formatPercent(s.margin_percent);
        row[m_summaryCols.col_stock] = s.current_stock;
    }
}

void MainWindow::refreshInventory() {
    m_invStore->clear();
    std::string filter = m_searchInventory.get_text();
    auto products = Database::instance().getProducts(filter);

    int totalUnits = 0;
    double totalInventoryCost = 0.0;
    double totalPotentialProfit = 0.0;

    for (const auto& p : products) {
        auto row = *(m_invStore->append());
        row[m_invCols.col_id] = p.id;
        row[m_invCols.col_sku] = p.sku;
        row[m_invCols.col_name] = p.name;
        row[m_invCols.col_brand] = p.brand;
        row[m_invCols.col_category] = p.category;
        row[m_invCols.col_cost] = formatHNL(p.cost_price);
        row[m_invCols.col_price] = formatDual(p.sale_price);
        row[m_invCols.col_unit_profit] = formatHNL(p.unit_profit());
        row[m_invCols.col_margin] = formatPercent(p.margin_percent());
        row[m_invCols.col_stock] = p.stock;
        row[m_invCols.col_status] = p.stock_status_emoji();

        totalUnits += p.stock;
        totalInventoryCost += (p.cost_price * p.stock);
        totalPotentialProfit += (p.unit_profit() * p.stock);
    }

    std::ostringstream ss;
    ss << "Mostrando " << products.size() << " productos. Total existencias: " << totalUnits
       << " uds | Inversión: " << formatDual(totalInventoryCost)
       << " | Ganancia potencial: " << formatDual(totalPotentialProfit);
    m_lblInventoryStats.set_text(ss.str());
}

void MainWindow::refreshPOSCatalog() {
    m_posProductCombo.remove_all();
    m_posProductList = Database::instance().getProducts("");

    for (const auto& p : m_posProductList) {
        std::string text = p.name + " (" + p.brand + ") - " + formatHNL(p.sale_price) + " [Stock: " + std::to_string(p.stock) + "]";
        m_posProductCombo.append(std::to_string(p.id), text);
    }

    if (!m_posProductList.empty()) {
        m_posProductCombo.set_active(0);
        onPosProductChanged();
    }
}

void MainWindow::refreshHistory() {
    m_histStore->clear();
    auto tf = getTimeFilterFromCombo(m_comboTimeFilter.get_active_id());
    auto list = Database::instance().getSalesHistory(tf);

    for (const auto& r : list) {
        auto row = *(m_histStore->append());
        row[m_histCols.col_id] = r.id;
        row[m_histCols.col_date] = r.date_time;
        row[m_histCols.col_items_count] = r.total_items;
        row[m_histCols.col_details] = r.items_summary;
        row[m_histCols.col_method] = r.payment_method;
        row[m_histCols.col_revenue] = formatDual(r.total_revenue);
        row[m_histCols.col_cost] = formatHNL(r.total_cost);
        row[m_histCols.col_profit] = formatDual(r.total_profit);
    }
}

void MainWindow::onExchangeRateChanged() {
    refreshAll();
    onPosDiscountChanged();
}

void MainWindow::onTimeFilterChanged() {
    refreshKPIs();
    refreshSummary();
    refreshHistory();
}

// ----------------- Product Management -----------------

void MainWindow::onAddProductClicked() {
    auto dialog = new ProductDialog(*this, "Agregar Producto a Barbería");
    dialog->set_on_save([this, dialog](const Product& p) {
        int newId = 0;
        std::string err;
        if (Database::instance().addProduct(p, newId, err)) {
            refreshAll();
            m_lblStatusAction.set_text("Producto agregado exitosamente: " + p.name);
            auto alert = Gtk::AlertDialog::create("✅ Producto Agregado");
            alert->set_detail("Se guardó '" + p.name + "' (" + p.brand + ") con código " + p.sku + " en la base de datos.");
            alert->show(*this);
        } else {
            auto alert = Gtk::AlertDialog::create("❌ Error al Guardar");
            alert->set_detail(err);
            alert->show(*this);
        }
        delete dialog;
    });
    dialog->present();
}

void MainWindow::onEditProductClicked() {
    auto selection = m_tvInventory.get_selection();
    auto iter = selection->get_selected();
    if (!iter) {
        auto alert = Gtk::AlertDialog::create("ℹ️ Selecciona un Producto");
        alert->set_detail("Por favor haz clic en un producto de la tabla para editarlo.");
        alert->show(*this);
        return;
    }

    int productId = (*iter)[m_invCols.col_id];
    auto optProd = Database::instance().getProductById(productId);
    if (!optProd) return;

    auto dialog = new ProductDialog(*this, "Editar Producto", &optProd.value());
    dialog->set_on_save([this, dialog](const Product& p) {
        std::string err;
        if (Database::instance().updateProduct(p, err)) {
            refreshAll();
            m_lblStatusAction.set_text("Producto actualizado: " + p.name);
            auto alert = Gtk::AlertDialog::create("✅ Producto Actualizado");
            alert->set_detail("Los cambios de '" + p.name + "' fueron guardados correctamente.");
            alert->show(*this);
        } else {
            auto alert = Gtk::AlertDialog::create("❌ Error al Actualizar");
            alert->set_detail(err);
            alert->show(*this);
        }
        delete dialog;
    });
    dialog->present();
}

void MainWindow::onDeleteProductClicked() {
    auto selection = m_tvInventory.get_selection();
    auto iter = selection->get_selected();
    if (!iter) {
        auto alert = Gtk::AlertDialog::create("ℹ️ Selecciona un Producto");
        alert->set_detail("Por favor haz clic en la fila del producto que deseas quitar.");
        alert->show(*this);
        return;
    }

    int productId = (*iter)[m_invCols.col_id];
    Glib::ustring productName = (*iter)[m_invCols.col_name];

    auto confirmAlert = Gtk::AlertDialog::create("¿Eliminar producto de barbería?");
    confirmAlert->set_detail("¿Estás seguro de que deseas quitar '" + std::string(productName) + "'?\n\nSi tiene ventas históricas asociadas, se archivará protegiendo las estadísticas contables.");
    std::vector<Glib::ustring> btns = {"Cancelar", "Eliminar"};
    confirmAlert->set_buttons(btns);

    confirmAlert->choose(*this, [this, confirmAlert, productId, productName](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            int selectedBtn = confirmAlert->choose_finish(res);
            if (selectedBtn == 1) {
                std::string err;
                if (Database::instance().deleteProduct(productId, err)) {
                    refreshAll();
                    m_lblStatusAction.set_text("Producto eliminado: " + std::string(productName));
                    auto okAlert = Gtk::AlertDialog::create("🗑️ Producto Quitado");
                    okAlert->set_detail("El producto '" + std::string(productName) + "' ha sido eliminado del catálogo.");
                    okAlert->show(*this);
                } else {
                    auto errAlert = Gtk::AlertDialog::create("❌ Error al eliminar");
                    errAlert->set_detail(err);
                    errAlert->show(*this);
                }
            }
        } catch (...) {}
    });
}

void MainWindow::onInventoryFilterChanged() {
    refreshInventory();
}

// ----------------- POS / Sales Handlers -----------------

void MainWindow::onPosProductChanged() {
    std::string activeIdStr = m_posProductCombo.get_active_id();
    if (activeIdStr.empty()) return;

    int pId = std::stoi(activeIdStr);
    for (const auto& p : m_posProductList) {
        if (p.id == pId) {
            m_posLblItemPrice.set_text(formatDual(p.sale_price));
            m_posLblStockInfo.set_text(std::to_string(p.stock) + " unidades disponibles");
            m_posSpinQty.set_range(1, std::max(1, p.stock));
            m_posSpinQty.set_value(1);
            break;
        }
    }
}

void MainWindow::onPosAddToCartClicked() {
    std::string activeIdStr = m_posProductCombo.get_active_id();
    if (activeIdStr.empty()) return;

    int pId = std::stoi(activeIdStr);
    int qty = static_cast<int>(m_posSpinQty.get_value());

    auto optProd = Database::instance().getProductById(pId);
    if (!optProd) return;
    Product prod = optProd.value();

    if (prod.stock < qty) {
        auto alert = Gtk::AlertDialog::create("⚠️ Stock Insuficiente");
        alert->set_detail("Solo hay " + std::to_string(prod.stock) + " unidades disponibles de '" + prod.name + "'.");
        alert->show(*this);
        return;
    }

    bool exists = false;
    for (auto& item : m_cartItems) {
        if (item.product.id == prod.id) {
            if (item.quantity + qty > prod.stock) {
                auto alert = Gtk::AlertDialog::create("⚠️ Stock Excedido");
                alert->set_detail("Ya tienes " + std::to_string(item.quantity) + " en el ticket. El stock total disponible es " + std::to_string(prod.stock) + ".");
                alert->show(*this);
                return;
            }
            item.quantity += qty;
            exists = true;
            break;
        }
    }

    if (!exists) {
        CartItem ci;
        ci.product = prod;
        ci.quantity = qty;
        m_cartItems.push_back(ci);
    }

    onPosDiscountChanged();
}

void MainWindow::onPosRemoveFromCartClicked() {
    auto selection = m_tvCart.get_selection();
    auto iter = selection->get_selected();
    if (!iter) return;

    int pId = (*iter)[m_cartCols.col_id];
    for (auto it = m_cartItems.begin(); it != m_cartItems.end(); ++it) {
        if (it->product.id == pId) {
            m_cartItems.erase(it);
            break;
        }
    }

    onPosDiscountChanged();
}

void MainWindow::onPosClearCartClicked() {
    m_cartItems.clear();
    m_cartStore->clear();
    m_posLblTotalPay.set_text("L 0.00");
    m_posLblTotalUSD.set_text("Equivalente en Dólares: $0.00 USD");
    m_posLblEstimatedProfit.set_text("Ganancia neta estimada: +L 0.00");
    m_posEntryCash.set_text("");
    m_posSpinDiscount.set_value(0.0);
    m_posLblChange.set_text("Cambio a devolver: L 0.00");
}

void MainWindow::onPosDiscountChanged() {
    m_cartStore->clear();
    double totalPay = 0.0;
    double totalProfit = 0.0;

    for (const auto& item : m_cartItems) {
        auto row = *(m_cartStore->append());
        row[m_cartCols.col_id] = item.product.id;
        row[m_cartCols.col_sku] = item.product.sku;
        row[m_cartCols.col_name] = item.product.name;
        row[m_cartCols.col_qty] = item.quantity;
        row[m_cartCols.col_unit_price] = formatDual(item.product.sale_price);
        row[m_cartCols.col_subtotal] = formatDual(item.subtotal());
        row[m_cartCols.col_profit] = formatDual(item.profit());

        totalPay += item.subtotal();
        totalProfit += item.profit();
    }

    double discount = m_posSpinDiscount.get_value();
    double finalTotal = std::max(0.0, totalPay - discount);
    double finalProfit = std::max(0.0, totalProfit - discount);

    m_posLblTotalPay.set_text(formatHNL(finalTotal));
    m_posLblTotalUSD.set_text("Equivalente en Dólares: " + formatUSD(finalTotal) + " USD (TC: " + std::to_string(getExchangeRate()).substr(0, 5) + ")");
    m_posLblEstimatedProfit.set_text("Ganancia neta estimada: +" + formatDual(finalProfit));

    onPosCashChanged();
}

void MainWindow::onPosCashChanged() {
    std::string text = m_posEntryCash.get_text();
    double totalPay = 0.0;
    for (const auto& it : m_cartItems) totalPay += it.subtotal();
    double finalTotal = std::max(0.0, totalPay - m_posSpinDiscount.get_value());

    if (text.empty()) {
        m_posLblChange.set_text("Cambio a devolver: L 0.00");
        return;
    }

    try {
        double cash = std::stod(text);
        double change = cash - finalTotal;
        if (change >= 0.0) {
            m_posLblChange.set_text("Cambio a devolver: " + formatDual(change));
        } else {
            m_posLblChange.set_text("Faltante por pagar: " + formatDual(-change));
        }
    } catch (...) {
        m_posLblChange.set_text("Cambio a devolver: L 0.00");
    }
}

void MainWindow::onPosCheckoutClicked() {
    if (m_cartItems.empty()) {
        auto alert = Gtk::AlertDialog::create("ℹ️ Ticket Vacío");
        alert->set_detail("Agrega al menos un producto al ticket antes de cobrar.");
        alert->show(*this);
        return;
    }

    double discount = m_posSpinDiscount.get_value();
    std::string method = m_posComboPayment.get_active_id();
    if (method.empty()) method = "Efectivo";

    int saleId = 0;
    std::string err;
    if (Database::instance().recordSale(m_cartItems, "Venta en mostrador", discount, method, saleId, err)) {
        double totalPaid = 0.0;
        double profitMade = 0.0;
        int itemsCount = 0;
        for (const auto& item : m_cartItems) {
            totalPaid += item.subtotal();
            profitMade += item.profit();
            itemsCount += item.quantity;
        }
        totalPaid = std::max(0.0, totalPaid - discount);
        profitMade = std::max(0.0, profitMade - discount);

        double cashGiven = 0.0;
        try {
            if (!m_posEntryCash.get_text().empty()) {
                cashGiven = std::stod(m_posEntryCash.get_text());
            }
        } catch (...) {}
        double changeDue = std::max(0.0, cashGiven - totalPaid);

        // Generate and optionally print/save receipt
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::stringstream ssTime;
        ssTime << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");

        std::string receiptText = ReceiptPrinter::generateReceipt(
            saleId, ssTime.str(), m_cartItems, discount, totalPaid, cashGiven, changeDue, method, getExchangeRate()
        );

        // Save receipt to file
        std::string receiptsDir = "recibos";
        ReceiptPrinter::saveReceiptToFile(receiptText, receiptsDir, saleId);

        // Print if auto print checked
        if (m_chkAutoPrintReceipt.get_active()) {
            ReceiptPrinter::printReceipt(receiptText);
        }

        std::string detailMsg = "Ticket #" + std::to_string(saleId) + " registrado con éxito.\n" +
                               "Total cobrado: " + formatDual(totalPaid) + "\n" +
                               "Ganancia generada: +" + formatDual(profitMade) + "\n" +
                               "Método de pago: " + method + "\n\n" +
                               "📄 Recibo guardado en: recibos/ticket_" + std::to_string(saleId) + ".txt";

        onPosClearCartClicked();
        refreshAll();

        auto alert = Gtk::AlertDialog::create("🎉 ¡Venta Cobrada Exitosamente!");
        alert->set_detail(detailMsg);
        alert->show(*this);
    } else {
        auto alert = Gtk::AlertDialog::create("❌ Error al Registrar Venta");
        alert->set_detail(err);
        alert->show(*this);
    }
}

// ----------------- Export Functions -----------------

void MainWindow::onExportSummaryClicked() {
    std::string path = "Resumen_Ganancias_Barberia.csv";
    auto tf = getTimeFilterFromCombo(m_comboTimeFilter.get_active_id());
    auto metrics = Database::instance().getMetrics(tf);
    auto stats = Database::instance().getProductSalesBreakdown("", tf);

    if (CsvExporter::exportFinancialSummary(path, metrics, stats)) {
        auto alert = Gtk::AlertDialog::create("📥 Exportación Exitosa a Excel");
        alert->set_detail("El resumen financiero de ventas y ganancias se guardó en:\n" + path + "\n\nPuedes abrirlo con Microsoft Excel o LibreOffice Calc.");
        alert->show(*this);
    }
}

void MainWindow::onExportInventoryClicked() {
    std::string path = "Inventario_Barberia.csv";
    auto prods = Database::instance().getProducts("");

    if (CsvExporter::exportInventory(path, prods)) {
        auto alert = Gtk::AlertDialog::create("📥 Exportación Exitosa");
        alert->set_detail("El catálogo e inventario completo se guardó en:\n" + path + "\n\nCompatible con Microsoft Excel.");
        alert->show(*this);
    }
}

void MainWindow::onExportHistoryClicked() {
    std::string path = "Historial_Ventas_Barberia.csv";
    auto tf = getTimeFilterFromCombo(m_comboTimeFilter.get_active_id());
    auto hist = Database::instance().getSalesHistory(tf);

    if (CsvExporter::exportSalesHistory(path, hist)) {
        auto alert = Gtk::AlertDialog::create("📥 Exportación Exitosa");
        alert->set_detail("El historial de ventas se guardó en:\n" + path + "\n\nCompatible con Microsoft Excel.");
        alert->show(*this);
    }
}

void MainWindow::onOpenReceiptFolderClicked() {
    std::string receiptsDir = "recibos";
    ReceiptPrinter::openReceiptFolder(receiptsDir);
    m_lblStatusAction.set_text("Carpeta de recibos abierta: " + receiptsDir);
}

void MainWindow::onPrintSelectedHistoryTicketClicked() {
    auto selection = m_tvHistory.get_selection();
    auto iter = selection->get_selected();
    if (!iter) {
        auto alert = Gtk::AlertDialog::create("ℹ️ Selecciona un Ticket");
        alert->set_detail("Haz clic sobre una venta del historial para imprimir su ticket o abrirlo.");
        alert->show(*this);
        return;
    }

    int saleId = (*iter)[m_histCols.col_id];
    std::string filePath = "recibos/ticket_" + std::to_string(saleId) + ".txt";

    // If the file does not exist, reconstruct it from sale details
    std::ifstream testFile(filePath);
    if (!testFile.good()) {
        auto items = Database::instance().getSaleDetails(saleId);
        std::vector<CartItem> cartItems;
        for (const auto& item : items) {
            CartItem ci;
            ci.product.name = item.product_name;
            ci.product.sku = item.sku;
            ci.product.sale_price = item.unit_price;
            ci.quantity = item.quantity;
            cartItems.push_back(ci);
        }
        Glib::ustring dateUStr = (*iter)[m_histCols.col_date];
        Glib::ustring methodUStr = (*iter)[m_histCols.col_method];
        std::string dateStr = std::string(dateUStr);
        std::string method = std::string(methodUStr);
        double rev = 0.0;
        for (const auto& ci : cartItems) rev += ci.subtotal();

        std::string receiptText = ReceiptPrinter::generateReceipt(
            saleId, dateStr, cartItems, 0.0, rev, rev, 0.0, method, getExchangeRate()
        );
        ReceiptPrinter::saveReceiptToFile(receiptText, "recibos", saleId);
    }

    // Attempt print
    bool printed = ReceiptPrinter::printReceiptFile(filePath);

    auto alert = Gtk::AlertDialog::create(printed ? "🖨️ Ticket Enviado a Impresora" : "📄 Ticket Listo en Carpeta");
    std::string msg = "Ticket #" + std::to_string(saleId) + "\nArchivo: " + filePath + "\n\n";
    if (printed) {
        msg += "Se envió el archivo a la cola de impresión.";
    } else {
        msg += "El archivo del ticket está guardado y listo en la carpeta 'recibos/'.";
    }
    alert->set_detail(msg);
    alert->show(*this);
}

void MainWindow::onDeleteHistorySaleClicked() {
    auto selection = m_tvHistory.get_selection();
    auto iter = selection->get_selected();
    if (!iter) {
        auto alert = Gtk::AlertDialog::create("ℹ️ Selecciona una Venta");
        alert->set_detail("Por favor haz clic en la venta del historial que deseas eliminar.");
        alert->show(*this);
        return;
    }

    int saleId = (*iter)[m_histCols.col_id];
    Glib::ustring dateStr = (*iter)[m_histCols.col_date];
    Glib::ustring revStr = (*iter)[m_histCols.col_revenue];

    auto confirmAlert = Gtk::AlertDialog::create("¿Eliminar Venta / Ticket?");
    std::string detail = "¿Estás seguro de que deseas eliminar permanentemente el Ticket #" + std::to_string(saleId) +
                         " (" + std::string(dateStr) + " - " + std::string(revStr) + ")?\n\n" +
                         "Las estadísticas de ventas e ingresos se recalcularán automáticamente.";
    confirmAlert->set_detail(detail);
    std::vector<Glib::ustring> btns = {"Cancelar", "Eliminar y Revertir Stock", "Eliminar Solamente"};
    confirmAlert->set_buttons(btns);

    confirmAlert->choose(*this, [this, confirmAlert, saleId](const Glib::RefPtr<Gio::AsyncResult>& res) {
        try {
            int selectedBtn = confirmAlert->choose_finish(res);
            if (selectedBtn == 1 || selectedBtn == 2) {
                bool restoreStock = (selectedBtn == 1);
                std::string err;
                if (Database::instance().deleteSale(saleId, restoreStock, err)) {
                    // Also remove receipt file if exists
                    std::string ticketFile = "recibos/ticket_" + std::to_string(saleId) + ".txt";
                    std::remove(ticketFile.c_str());

                    refreshAll();
                    m_lblStatusAction.set_text("Venta #" + std::to_string(saleId) + " eliminada.");

                    auto okAlert = Gtk::AlertDialog::create("🗑️ Venta Eliminada");
                    std::string okMsg = "El Ticket #" + std::to_string(saleId) + " fue eliminado del sistema.\n";
                    if (restoreStock) {
                        okMsg += "Los productos vendidos fueron devueltos al inventario disponible.";
                    } else {
                        okMsg += "No se modificó el inventario existente.";
                    }
                    okAlert->set_detail(okMsg);
                    okAlert->show(*this);
                } else {
                    auto errAlert = Gtk::AlertDialog::create("❌ Error al Eliminar Venta");
                    errAlert->set_detail(err);
                    errAlert->show(*this);
                }
            }
        } catch (...) {}
    });
}
