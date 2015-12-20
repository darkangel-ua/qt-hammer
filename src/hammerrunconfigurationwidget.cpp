#include "hammerrunconfigurationwidget.h"
#include "hammerrunconfiguration.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

namespace hammer{ namespace QtCreator{

HammerRunConfigurationWidget::HammerRunConfigurationWidget(HammerRunConfiguration *rc, ApplyMode mode)
    : m_ignoreChange(false), m_runConfiguration(rc)
{
    QFormLayout *layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_commandLineArgumentsLineEdit = new QLineEdit(this);
    m_commandLineArgumentsLineEdit->setMinimumWidth(200); // this shouldn't be fixed here...
    layout->addRow(tr("Arguments:"), m_commandLineArgumentsLineEdit);

    m_workingDirectory = new Utils::PathChooser(this);
    m_workingDirectory->setHistoryCompleter(QLatin1String("Qt.WorkingDir.History"));
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectory->setBaseFileName(rc->target()->project()->projectDirectory());

    layout->addRow(tr("Working directory:"), m_workingDirectory);

    m_useTerminalCheck = new QCheckBox(tr("Run in &terminal"), this);
    layout->addRow(QString(), m_useTerminalCheck);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    changed();

    if (mode == InstantApply) {
        connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(argumentsEdited(QString)));
        connect(m_workingDirectory, SIGNAL(changed(QString)),
                this, SLOT(workingDirectoryEdited()));
        connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
                this, SLOT(termToggled(bool)));
    } else {
        connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(QString)),
                this, SIGNAL(validChanged()));
        connect(m_workingDirectory, SIGNAL(changed(QString)),
                this, SIGNAL(validChanged()));
        connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
                this, SIGNAL(validChanged()));
    }

    ProjectExplorer::EnvironmentAspect *aspect = rc->extraAspect<ProjectExplorer::EnvironmentAspect>();
    if (aspect) {
        connect(aspect, SIGNAL(environmentChanged()), this, SLOT(environmentWasChanged()));
        environmentWasChanged();
    }

    // If we are in mode InstantApply, we keep us in sync with the rc
    // otherwise we ignore changes to the rc and override them on apply,
    // or keep them on cancel
    if (mode == InstantApply)
        connect(m_runConfiguration, SIGNAL(changed()), this, SLOT(changed()));

    Core::VariableChooser::addSupportForChildWidgets(this, m_runConfiguration->macroExpander());
}

void HammerRunConfigurationWidget::environmentWasChanged()
{
    ProjectExplorer::EnvironmentAspect *aspect
            = m_runConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectory->setEnvironment(aspect->environment());
}

void HammerRunConfigurationWidget::argumentsEdited(const QString &arguments)
{
    m_ignoreChange = true;
    m_runConfiguration->setCommandLineArguments(arguments);
    m_ignoreChange = false;
}

void HammerRunConfigurationWidget::workingDirectoryEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_ignoreChange = false;
}

void HammerRunConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_runConfiguration->setRunMode(on ? ProjectExplorer::ApplicationLauncher::Console
                                      : ProjectExplorer::ApplicationLauncher::Gui);
    m_ignoreChange = false;
}

void HammerRunConfigurationWidget::changed()
{
    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;

    m_commandLineArgumentsLineEdit->setText(m_runConfiguration->rawCommandLineArguments());
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
    m_useTerminalCheck->setChecked(m_runConfiguration->runMode()
                                   == ProjectExplorer::ApplicationLauncher::Console);
}

void HammerRunConfigurationWidget::apply()
{
    m_ignoreChange = true;
    m_runConfiguration->setCommandLineArguments(m_commandLineArgumentsLineEdit->text());
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_runConfiguration->setRunMode(m_useTerminalCheck->isChecked() ? ProjectExplorer::ApplicationLauncher::Console
                                                                   : ProjectExplorer::ApplicationLauncher::Gui);
    m_ignoreChange = false;
}

}}
