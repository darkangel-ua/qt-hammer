#include "hammerrunconfigurationwidget.h"
#include "hammerrunconfiguration.h"

#include <coreplugin/variablechooser.h>
#include <coreplugin/coreconstants.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
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
#include <QToolButton>

using namespace ProjectExplorer;

namespace hammer{ namespace QtCreator{

HammerRunConfigurationWidget::HammerRunConfigurationWidget(HammerRunConfiguration *rc)
    : m_ignoreChange(false),
      m_runConfiguration(rc)
{
    QFormLayout* layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_runConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, layout);

    m_workingDirectory = new Utils::PathChooser(this);
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectory->setBaseFileName(rc->target()->project()->projectDirectory());
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
    m_workingDirectory->setHistoryCompleter(QLatin1String("Qt.WorkingDir.History"));
    m_workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to Default"));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->addWidget(m_workingDirectory);
    boxlayout->addWidget(resetButton);

    layout->addRow(tr("Working directory:"), boxlayout);

    m_runConfiguration->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, layout);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
//    boxlayout->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    QVBoxLayout *vbx = new QVBoxLayout(this);
    vbx->setMargin(0);
    vbx->addWidget(m_detailsContainer);

    connect(m_workingDirectory, &Utils::PathChooser::rawPathChanged,
            this, &HammerRunConfigurationWidget::setWorkingDirectory);

    connect(resetButton, &QToolButton::clicked,
            this, &HammerRunConfigurationWidget::resetWorkingDirectory);

    connect(m_runConfiguration, &HammerRunConfiguration::baseWorkingDirectoryChanged,
            this, &HammerRunConfigurationWidget::workingDirectoryChanged);
}

void HammerRunConfigurationWidget::setWorkingDirectory()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_ignoreChange = false;
}

void HammerRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange) {
        m_ignoreChange = true;
        m_workingDirectory->setPath(workingDirectory);
        m_ignoreChange = false;
    }
}

void HammerRunConfigurationWidget::resetWorkingDirectory()
{
    m_runConfiguration->setBaseWorkingDirectory(QString());
}

void HammerRunConfigurationWidget::environmentWasChanged()
{
    EnvironmentAspect *aspect = m_runConfiguration->extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectory->setEnvironment(aspect->environment());
}

}}
