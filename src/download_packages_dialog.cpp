#include "download_packages_dialog.h"
#include "ui_download_packages_dialog.h"
#include <QStandardItemModel>
#include <QPushButton>
#include <QMessageBox>
#include <hammer/core/engine.h>
#include <boost/format.hpp>

using namespace std;

// http://stackoverflow.com/questions/3758606/how-to-convert-byte-size-into-human-readable-format-in-java
static
string human_readable_byte_count(const long long bytes,
                                 const bool si = true)
{
   const unsigned unit = si ? 1000 : 1024;
   if (bytes < unit)
      return (boost::format("%1 B") % bytes).str();

   const long exp = lrint(log(bytes) / log(unit));
   return (boost::format("%.1f %c%sB")
            % (bytes / pow(unit, exp))
            % ((si ? "kMGTPE" : "KMGTPE")[exp - 1])
            % (si ? "" : "i")).str();
}

download_packages_dialog::download_packages_dialog(QWidget *parent,
                                                   hammer::engine& engine,
                                                   const hammer::warehouse::package_infos_t& packages_to_download)
   : QDialog(parent),
     ui(new Ui::download_packages_dialog),
     engine_(engine),
     packages_to_download_(packages_to_download)
{
   ui->setupUi(this);

   model_ = new QStandardItemModel(packages_to_download.size(), 4, this);

   int row = 0;
   for(const auto& p : packages_to_download_) {
      model_->setItem(row, 0, new QStandardItem(QString::fromStdString(p.name_)));
      model_->setItem(row, 1, new QStandardItem(QString::fromStdString(p.version_)));
      model_->setItem(row, 2, new QStandardItem(QString::fromStdString(human_readable_byte_count(p.package_file_size_))));

      ++row;
   }

   ui->packages_->setModel(model_);
   ui->packages_->resizeColumnsToContents();
   ui->packages_->resizeRowsToContents();
   model_->setHeaderData(0, Qt::Horizontal, QString("Name"), Qt::DisplayRole);
   model_->setHeaderData(1, Qt::Horizontal, QString("Version"), Qt::DisplayRole);
   model_->setHeaderData(2, Qt::Horizontal, QString("Size"), Qt::DisplayRole);
   model_->setHeaderData(3, Qt::Horizontal, QString("Progress"), Qt::DisplayRole);
}

download_packages_dialog::~download_packages_dialog()
{
   delete ui;
}

void download_packages_dialog::on_status_changed(const int index,
                                                 const QString& status)
{
   model_->setItem(index, 3, new QStandardItem(status));
}

void download_packages_dialog::on_download_finished()
{
   // if Cancel button active than user didn't interrupted process and everything ok
   done(ui->buttonBox->button(QDialogButtonBox::Cancel)->isEnabled() ? QDialog::Accepted : QDialog::Rejected);
}

void download_packages_dialog::on_download_error(const QString& error)
{
   QMessageBox::critical(this, "Error", error);
   done(QDialog::Rejected);
}

void download_packages_dialog::accept()
{
   ui->buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
   download_thread_ = new download_thread(*this);

   connect(download_thread_, &download_thread::status_changed,
           this, &download_packages_dialog::on_status_changed);

   connect(download_thread_, &download_thread::finished,
           this, &download_packages_dialog::on_download_finished);

   download_thread_->start();
}

void download_packages_dialog::reject()
{
   if (download_thread_) {
      download_thread_->requestInterruption();
      ui->buttonBox->button(QDialogButtonBox::Cancel)->setDisabled(true);
   } else
      QDialog::reject();
}

void download_packages_dialog::download_thread::run()
{
   try {
      owner_.engine_.warehouse().download_and_install(owner_.engine_, owner_.packages_to_download_, *this);
   } catch (const std::exception& e) {
      emit error_happened(e.what());
   }
}

bool download_packages_dialog::download_thread::on_download_begin(const std::size_t index,
                                                                  const hammer::warehouse::package_info& package)
{
   if (isInterruptionRequested())
      return false;

   emit status_changed(index, "Downloading...");

   return true;
}

void download_packages_dialog::download_thread::on_download_end(const std::size_t index,
                                                                const hammer::warehouse::package_info& package)
{
   emit status_changed(index, "Downloaded");
}

bool download_packages_dialog::download_thread::on_install_begin(const std::size_t index,
                                                                 const hammer::warehouse::package_info& package)
{
   if (isInterruptionRequested())
      return false;

   emit status_changed(index, "Installing...");

   return true;
}

void download_packages_dialog::download_thread::on_install_end(const std::size_t index,
                                                               const hammer::warehouse::package_info& package)
{
   emit status_changed(index, "Installed");
}
