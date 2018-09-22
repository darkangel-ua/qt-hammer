#pragma once
#include <QDialog>
#include <QThread>
#include <hammer/core/warehouse.h>

namespace Ui {

class download_packages_dialog;

}

class QStandardItemModel;

class download_packages_dialog : public QDialog {
      Q_OBJECT

   public:
      explicit download_packages_dialog(QWidget* parent,
                                        hammer::engine& engine,
                                        const hammer::warehouse::package_infos_t& packages_to_download);
      ~download_packages_dialog();

   private slots:
      void on_status_changed(const int index,
                             const QString& status);
      void on_download_finished();
      void on_download_error(const QString& error);

   private:
      class download_thread;

      Ui::download_packages_dialog *ui;
      hammer::engine& engine_;
      const hammer::warehouse::package_infos_t packages_to_download_;
      QStandardItemModel* model_;
      download_thread* download_thread_ = nullptr;

      void accept() override;
      void reject() override;
};

class download_packages_dialog::download_thread : public QThread,
                                                  private hammer::iwarehouse_download_and_install
{
      Q_OBJECT

   public:
      download_thread(download_packages_dialog& owner)
         : QThread(&owner),
           owner_(owner)
      {}

      void run() override;

   signals:
      void status_changed(const int index,
                          const QString& status);
      void error_happened(const QString& error);

   private:
      download_packages_dialog& owner_;

      bool on_download_begin(const std::size_t index,
                             const hammer::warehouse::package_info& package) override;
      void on_download_end(const std::size_t index,
                           const hammer::warehouse::package_info& package) override;
      bool on_install_begin(const std::size_t index,
                            const hammer::warehouse::package_info& package) override;
      void on_install_end(const std::size_t index,
                          const hammer::warehouse::package_info& package) override;
};
