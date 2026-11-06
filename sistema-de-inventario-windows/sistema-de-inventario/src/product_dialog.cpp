#include "product_dialog.hpp"
#include <random>
#include <sstream>

ProductDialog::ProductDialog(Gtk::Window& parent, const std::string& title, const Product* existingProduct) {
    set_transient_for(parent);
    set_modal(true);
    set_title(title);
    set_default_size(500, 520);
    set_resizable(false);

    if (existingProduct) {
        m_isEdit = true;
        m_productId = existingProduct->id;
    }

    setupUI();

    if (existingProduct) {
        m_entrySku.set_text(existingProduct->sku);
        m_entryName.set_text(existingProduct->name);
        m_entryBrand.set_text(existingProduct->brand);
        m_comboCategory.set_active_text(existingProduct->category);
        m_spinCost.set_value(existingProduct->cost_price);
        m_spinPrice.set_value(existingProduct->sale_price);
        m_spinStock.set_value(existingProduct->stock);
        m_spinMinStock.set_value(existingProduct->min_stock);
        m_btnSave.set_label("💾 Actualizar Producto");
    } else {
        onAutoSkuClicked();
        m_entryBrand.set_text("Suavecito");
        m_comboCategory.set_active(0);
        m_spinCost.set_value(100.0);
        m_spinPrice.set_value(250.0);
        m_spinStock.set_value(15);
        m_spinMinStock.set_value(5);
    }

    updateCalculations();
}

void ProductDialog::setupUI() {
    m_mainBox.set_margin(20);

    // Header label
    auto headerBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::VERTICAL, 4);
    auto lblHeader = Gtk::make_managed<Gtk::Label>(m_isEdit ? "Editar Producto de Barbería" : "Agregar Nuevo Producto");
    lblHeader->add_css_class("dialog-title");
    auto lblSub = Gtk::make_managed<Gtk::Label>("Ingresa los detalles, precios en Lempiras (HNL) y stock para calcular márgenes.");
    lblSub->add_css_class("dialog-subtitle");
    headerBox->append(*lblHeader);
    headerBox->append(*lblSub);
    m_mainBox.append(*headerBox);

    // Form grid
    m_grid.set_row_spacing(10);
    m_grid.set_column_spacing(12);
    m_grid.set_margin_top(10);
    m_grid.set_margin_bottom(10);

    // SKU row
    auto lblSku = Gtk::make_managed<Gtk::Label>("Código / SKU:");
    lblSku->set_halign(Gtk::Align::START);
    auto skuBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    m_entrySku.set_hexpand(true);
    m_entrySku.set_placeholder_text("Ej. CER-001");
    m_btnGenSku.signal_clicked().connect(sigc::mem_fun(*this, &ProductDialog::onAutoSkuClicked));
    skuBox->append(m_entrySku);
    skuBox->append(m_btnGenSku);

    m_grid.attach(*lblSku, 0, 0);
    m_grid.attach(*skuBox, 1, 0);

    // Name row
    auto lblName = Gtk::make_managed<Gtk::Label>("Nombre del Producto:");
    lblName->set_halign(Gtk::Align::START);
    m_entryName.set_hexpand(true);
    m_entryName.set_placeholder_text("Ej. Cera Mate Fijación Fuerte");
    m_grid.attach(*lblName, 0, 1);
    m_grid.attach(m_entryName, 1, 1);

    // Brand row
    auto lblBrand = Gtk::make_managed<Gtk::Label>("Marca / Proveedor:");
    lblBrand->set_halign(Gtk::Align::START);
    m_entryBrand.set_hexpand(true);
    m_entryBrand.set_placeholder_text("Ej. Suavecito, Proraso, Reuzel");
    m_grid.attach(*lblBrand, 0, 2);
    m_grid.attach(m_entryBrand, 1, 2);

    // Category row
    auto lblCat = Gtk::make_managed<Gtk::Label>("Categoría:");
    lblCat->set_halign(Gtk::Align::START);
    m_comboCategory.append("Pomadas y Ceras");
    m_comboCategory.append("Cuidado de Barba");
    m_comboCategory.append("Afeitado");
    m_comboCategory.append("Cuidado Capilar");
    m_comboCategory.append("Herramientas");
    m_comboCategory.append("Fragancias");
    m_comboCategory.append("Otros");
    m_comboCategory.set_hexpand(true);
    m_grid.attach(*lblCat, 0, 3);
    m_grid.attach(m_comboCategory, 1, 3);

    // Cost price row
    auto lblCost = Gtk::make_managed<Gtk::Label>("Costo Adquisición (L):");
    lblCost->set_halign(Gtk::Align::START);
    m_spinCost.set_range(0.01, 100000.0);
    m_spinCost.set_increments(5.0, 50.0);
    m_spinCost.set_digits(2);
    m_spinCost.set_hexpand(true);
    m_spinCost.signal_value_changed().connect(sigc::mem_fun(*this, &ProductDialog::updateCalculations));
    m_grid.attach(*lblCost, 0, 4);
    m_grid.attach(m_spinCost, 1, 4);

    // Sale price row
    auto lblPrice = Gtk::make_managed<Gtk::Label>("Precio Venta (L):");
    lblPrice->set_halign(Gtk::Align::START);
    m_spinPrice.set_range(0.01, 100000.0);
    m_spinPrice.set_increments(5.0, 50.0);
    m_spinPrice.set_digits(2);
    m_spinPrice.set_hexpand(true);
    m_spinPrice.signal_value_changed().connect(sigc::mem_fun(*this, &ProductDialog::updateCalculations));
    m_grid.attach(*lblPrice, 0, 5);
    m_grid.attach(m_spinPrice, 1, 5);

    // Stock row
    auto lblStock = Gtk::make_managed<Gtk::Label>("Stock Disponible (Uds):");
    lblStock->set_halign(Gtk::Align::START);
    m_spinStock.set_range(0, 10000);
    m_spinStock.set_increments(1, 10);
    m_spinStock.set_digits(0);
    m_spinStock.set_hexpand(true);
    m_grid.attach(*lblStock, 0, 6);
    m_grid.attach(m_spinStock, 1, 6);

    // Min Stock row
    auto lblMinStock = Gtk::make_managed<Gtk::Label>("Alerta Stock Mínimo:");
    lblMinStock->set_halign(Gtk::Align::START);
    m_spinMinStock.set_range(1, 1000);
    m_spinMinStock.set_increments(1, 5);
    m_spinMinStock.set_digits(0);
    m_spinMinStock.set_hexpand(true);
    m_grid.attach(*lblMinStock, 0, 7);
    m_grid.attach(m_spinMinStock, 1, 7);

    m_mainBox.append(m_grid);

    // Real-time calculation card
    auto calcFrame = Gtk::make_managed<Gtk::Frame>();
    calcFrame->add_css_class("calc-preview-card");
    auto calcBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 20);
    calcBox->set_margin(12);
    calcBox->set_halign(Gtk::Align::CENTER);

    m_lblUnitProfit.add_css_class("preview-metric-profit");
    m_lblMargin.add_css_class("preview-metric-margin");

    calcBox->append(m_lblUnitProfit);
    calcBox->append(m_lblMargin);
    calcFrame->set_child(*calcBox);
    m_mainBox.append(*calcFrame);

    // Error label
    m_lblError.set_halign(Gtk::Align::CENTER);
    m_lblError.add_css_class("error-text");
    m_mainBox.append(m_lblError);

    // Buttons row
    auto btnBox = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 12);
    btnBox->set_halign(Gtk::Align::END);
    btnBox->set_margin_top(8);

    m_btnCancel.add_css_class("btn-cancel");
    m_btnCancel.signal_clicked().connect(sigc::mem_fun(*this, &ProductDialog::onCancelClicked));

    m_btnSave.add_css_class("btn-action-primary");
    m_btnSave.signal_clicked().connect(sigc::mem_fun(*this, &ProductDialog::onSaveClicked));

    btnBox->append(m_btnCancel);
    btnBox->append(m_btnSave);
    m_mainBox.append(*btnBox);

    set_child(m_mainBox);
}

void ProductDialog::updateCalculations() {
    double cost = m_spinCost.get_value();
    double price = m_spinPrice.get_value();
    double profit = price - cost;
    double margin = price > 0.0 ? (profit / price) * 100.0 : 0.0;

    std::ostringstream ssP, ssM;
    ssP << "💰 Ganancia/Unidad: L " << std::fixed << std::setprecision(2) << profit;
    ssM << "📈 Margen: " << std::fixed << std::setprecision(1) << margin << "%";

    m_lblUnitProfit.set_text(ssP.str());
    m_lblMargin.set_text(ssM.str());
}

void ProductDialog::onAutoSkuClicked() {
    static std::mt19937 rng(1337);
    int num = 100 + (rng() % 900);
    std::string prefix = "BAR";
    std::string cat = m_comboCategory.get_active_text();
    if (cat.find("Cera") != std::string::npos || cat.find("Pomada") != std::string::npos) prefix = "CER";
    else if (cat.find("Barba") != std::string::npos) prefix = "BRB";
    else if (cat.find("Afeitado") != std::string::npos) prefix = "AFE";
    else if (cat.find("Capilar") != std::string::npos) prefix = "CAP";
    else if (cat.find("Herramientas") != std::string::npos) prefix = "HER";
    else if (cat.find("Fragancias") != std::string::npos) prefix = "FRA";

    m_entrySku.set_text(prefix + "-" + std::to_string(num));
}

void ProductDialog::onSaveClicked() {
    std::string sku = m_entrySku.get_text();
    std::string name = m_entryName.get_text();
    std::string brand = m_entryBrand.get_text();
    std::string category = m_comboCategory.get_active_text();
    double cost = m_spinCost.get_value();
    double price = m_spinPrice.get_value();
    int stock = static_cast<int>(m_spinStock.get_value());
    int minStock = static_cast<int>(m_spinMinStock.get_value());

    if (sku.empty()) {
        m_lblError.set_text("⚠️ El código SKU no puede estar vacío.");
        return;
    }
    if (name.empty()) {
        m_lblError.set_text("⚠️ Por favor ingresa el nombre del producto.");
        return;
    }
    if (category.empty()) {
        m_lblError.set_text("⚠️ Por favor selecciona una categoría.");
        return;
    }
    if (cost < 0.0) {
        m_lblError.set_text("⚠️ El costo no puede ser negativo.");
        return;
    }
    if (price <= 0.0) {
        m_lblError.set_text("⚠️ El precio de venta debe ser mayor a 0.");
        return;
    }

    Product p;
    p.id = m_productId;
    p.sku = sku;
    p.name = name;
    p.brand = brand.empty() ? "Genérico" : brand;
    p.category = category;
    p.cost_price = cost;
    p.sale_price = price;
    p.stock = stock;
    p.min_stock = minStock;

    if (m_on_save) {
        m_on_save(p);
    }
    close();
}

void ProductDialog::onCancelClicked() {
    close();
}
