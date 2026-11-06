#ifndef PRODUCT_DIALOG_HPP
#define PRODUCT_DIALOG_HPP

#include "models.hpp"
#include <gtkmm.h>
#include <functional>

class ProductDialog : public Gtk::Window {
public:
    ProductDialog(Gtk::Window& parent, const std::string& title, const Product* existingProduct = nullptr);

    void set_on_save(std::function<void(const Product&)> callback) {
        m_on_save = callback;
    }

private:
    void setupUI();
    void updateCalculations();
    void onSaveClicked();
    void onCancelClicked();
    void onAutoSkuClicked();

    bool m_isEdit = false;
    int m_productId = 0;

    Gtk::Box m_mainBox{Gtk::Orientation::VERTICAL, 16};
    Gtk::Grid m_grid;

    Gtk::Entry m_entrySku;
    Gtk::Button m_btnGenSku{"🎲 Auto"};
    Gtk::Entry m_entryName;
    Gtk::Entry m_entryBrand;
    Gtk::ComboBoxText m_comboCategory;
    Gtk::SpinButton m_spinCost;
    Gtk::SpinButton m_spinPrice;
    Gtk::SpinButton m_spinStock;
    Gtk::SpinButton m_spinMinStock;

    Gtk::Label m_lblUnitProfit;
    Gtk::Label m_lblMargin;
    Gtk::Label m_lblError;

    Gtk::Button m_btnSave{"💾 Guardar Producto"};
    Gtk::Button m_btnCancel{"❌ Cancelar"};

    std::function<void(const Product&)> m_on_save;
};

#endif // PRODUCT_DIALOG_HPP
