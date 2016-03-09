#ifndef HAMMERRUNCONFIGURATIONWIDGET_H
#define HAMMERRUNCONFIGURATIONWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLineEdit;
class QComboBox;
class QLabel;
class QAbstractButton;
QT_END_NAMESPACE

namespace Utils {
class DetailsWidget;
class PathChooser;
}

namespace hammer{ namespace QtCreator{

class HammerRunConfiguration;

class HammerRunConfigurationWidget : public QWidget
{
      Q_OBJECT

   public:
      HammerRunConfigurationWidget(HammerRunConfiguration* rc);

   private slots:
       void setWorkingDirectory();
       void resetWorkingDirectory();
       void environmentWasChanged();

       void workingDirectoryChanged(const QString &workingDirectory);

   private:
      bool m_ignoreChange;
      HammerRunConfiguration* m_runConfiguration;
      Utils::PathChooser *m_workingDirectory;
      Utils::DetailsWidget *m_detailsContainer;
};

}}

#endif
