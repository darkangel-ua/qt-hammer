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
      enum ApplyMode { InstantApply, DelayedApply};
      HammerRunConfigurationWidget(HammerRunConfiguration* rc, ApplyMode mode);
      void apply(); // only used for DelayedApply

   private slots:
      void changed();

      void argumentsEdited(const QString &arguments);
      void workingDirectoryEdited();
      void termToggled(bool);
      void environmentWasChanged();

   private:
      bool m_ignoreChange;
      HammerRunConfiguration* m_runConfiguration;
      QLineEdit *m_commandLineArgumentsLineEdit;
      Utils::PathChooser *m_workingDirectory;
      QCheckBox *m_useTerminalCheck;
      Utils::DetailsWidget *m_detailsContainer;
};

}}

#endif
