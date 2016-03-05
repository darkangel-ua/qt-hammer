#include "hammerbuildconfiguration.h"

#include "hammermakestep.h"
#include "hammerproject.h"
//#include "hammertarget.h"
#include "hammerprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/buildsteplist.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QInputDialog>

using ProjectExplorer::BuildConfiguration;

namespace hammer{ namespace QtCreator{

HammerBuildConfiguration::HammerBuildConfiguration(ProjectExplorer::Target *parent)
    : BuildConfiguration(parent, Core::Id(HAMMER_BC_ID))
{
}

HammerBuildConfiguration::HammerBuildConfiguration(ProjectExplorer::Target* parent,
                                                   HammerBuildConfiguration* source)
   : BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

HammerBuildConfiguration::~HammerBuildConfiguration()
{
}

ProjectExplorer::NamedWidget*
HammerBuildConfiguration::createConfigWidget()
{
   return new HammerBuildSettingsWidget(this);
}

//HammerTarget *HammerBuildConfiguration::hammerTarget() const
//{
//    return static_cast<HammerTarget *>(target());
//}

HammerBuildConfigurationFactory::HammerBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

HammerBuildConfigurationFactory::~HammerBuildConfigurationFactory()
{
}

//bool HammerBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
//{
//    if (!qobject_cast<HammerTarget *>(parent))
//        return false;
    
//    if (id == QLatin1String(HAMMER_BC_ID))
//        return true;
    
//    return false;
//}

ProjectExplorer::BuildInfo*
HammerBuildConfigurationFactory::createBuildInfo(const ProjectExplorer::Kit* k,
                                                 const Utils::FileName& buildDir) const
{
    ProjectExplorer::BuildInfo *info = new ProjectExplorer::BuildInfo(this);
    info->typeName = tr("Build");
    info->buildDirectory = buildDir;
    info->kitId = k->id();
    return info;
}

int HammerBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo*>
HammerBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target *parent) const
{
    QList<ProjectExplorer::BuildInfo *> result;
    ProjectExplorer::BuildInfo *info = createBuildInfo(parent->kit(), parent->project()->projectDirectory());
    result << info;
    return result;
}

int HammerBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k,
                                              const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(HAMMERMIMETYPE)))
        return 0;
    return -1;
}

QList<ProjectExplorer::BuildInfo*>
HammerBuildConfigurationFactory::availableSetups(const ProjectExplorer::Kit* k,
                                                 const QString& projectPath) const
{
    QList<ProjectExplorer::BuildInfo*> result;
    ProjectExplorer::BuildInfo *info = createBuildInfo(k, ProjectExplorer::Project::projectDirectory(Utils::FileName::fromString(projectPath)));
    //: The name of the build configuration created by default for a generic project.
    info->displayName = tr("Default");
    result << info;
    return result;
}

HammerBuildConfiguration*
HammerBuildConfigurationFactory::create(ProjectExplorer::Target* parent,
                                        const ProjectExplorer::BuildInfo* info) const
{
   QTC_ASSERT(info->factory() == this, return 0);
   QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
   QTC_ASSERT(!info->displayName.isEmpty(), return 0);

   HammerBuildConfiguration *bc = new HammerBuildConfiguration(parent);
   bc->setDisplayName(info->displayName);
   bc->setDefaultDisplayName(info->displayName);
   bc->setBuildDirectory(info->buildDirectory);

   ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
//   ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

   Q_ASSERT(buildSteps);
   HammerMakeStep *makeStep = new HammerMakeStep(buildSteps);
   buildSteps->insertStep(0, makeStep);
   makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);

//   Q_ASSERT(cleanSteps);
//   HammerMakeStep *cleanMakeStep = new HammerMakeStep(cleanSteps);
//   cleanSteps->insertStep(0, cleanMakeStep);
//   cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
//   cleanMakeStep->setClean(true);

   return bc;

//   HammerTarget *target(static_cast<HammerTarget *>(parent));

//    //TODO asking for name is duplicated everywhere, but maybe more
//    // wizards will show up, that incorporate choosing the name
//    bool ok;
//    QString buildConfigurationName = QInputDialog::getText(0,
//                          tr("New Configuration"),
//                          tr("New configuration name:"),
//                          QLineEdit::Normal,
//                          QString(),
//                          &ok);
//    if (!ok || buildConfigurationName.isEmpty())
//        return NULL;

//    HammerBuildConfiguration *bc = new HammerBuildConfiguration(target);
//    // we need to add current file build list, and toMap/fromMap is only the way for now
//    QVariantMap saved_bc = bc->toMap();
//    bc->fromMap(saved_bc);

//    {
//       ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
//       Q_ASSERT(buildSteps);
//       HammerMakeStep *makeStep = new HammerMakeStep(buildSteps);
//       buildSteps->insertStep(0, makeStep);
//       makeStep->setBuildTarget("all", /*on=*/true);

//       target->addBuildConfiguration(bc);
//    }

//    {
//       ProjectExplorer::BuildStepList *buildSteps = bc->stepList(HAMMER_BC_BUILD_CURRENT_LIST_ID);
//       Q_ASSERT(buildSteps);
//       HammerMakeCurrentStep *makeStep = new HammerMakeCurrentStep(buildSteps);
//       buildSteps->insertStep(0, makeStep);
//       target->addBuildConfiguration(bc);
//    }

//    return bc;
}

// from QtCreator projectexplorer/buildconfiguration.cpp
static const char* const BUILD_STEP_LIST_COUNT("ProjectExplorer.BuildConfiguration.BuildStepListCount");
static const char* const BUILD_STEP_LIST_PREFIX("ProjectExplorer.BuildConfiguration.BuildStepList.");

//QVariantMap HammerBuildConfiguration::toMap() const
//{
//   QVariantMap result = BuildConfiguration::toMap();

//   // we need to add current file build step coz there is no other way to do except toMap/fromMap
//   if (stepList(HAMMER_BC_BUILD_CURRENT_LIST_ID) == NULL)
//   {
//      result[BUILD_STEP_LIST_COUNT] = result[BUILD_STEP_LIST_COUNT].toInt() + 1;
//      ProjectExplorer::BuildStepList* bsl = new ProjectExplorer::BuildStepList(const_cast<HammerBuildConfiguration*>(this), HAMMER_BC_BUILD_CURRENT_LIST_ID);
//      result[QString(BUILD_STEP_LIST_PREFIX) + QString::number(result[BUILD_STEP_LIST_COUNT].toInt() - 1)] = bsl->toMap();
//      delete bsl;
//   }

//   return result;
//}

//bool HammerBuildConfiguration::fromMap(const QVariantMap &map)
//{
//   return ProjectExplorer::BuildConfiguration::fromMap(map);
//}

bool HammerBuildConfigurationFactory::canHandle(const ProjectExplorer::Target* t) const
{
   if (!t->project()->supportsKit(t->kit()))
        return false;

    return qobject_cast<HammerProject*>(t->project());
}

bool HammerBuildConfigurationFactory::canClone(const ProjectExplorer::Target* parent,
                                               ProjectExplorer::BuildConfiguration* source) const
{
   if (!canHandle(parent))
       return false;

   return source->id() == HAMMER_BC_ID;
}

HammerBuildConfiguration*
HammerBuildConfigurationFactory::clone(ProjectExplorer::Target* parent,
                                       BuildConfiguration* source)
{
   if (!canClone(parent, source))
       return 0;

   return new HammerBuildConfiguration(parent, qobject_cast<HammerBuildConfiguration*>(source));
}

bool HammerBuildConfigurationFactory::canRestore(const ProjectExplorer::Target* parent,
                                                 const QVariantMap& map) const
{
   if (!canHandle(parent))
       return false;

   return ProjectExplorer::idFromMap(map) == HAMMER_BC_ID;
}

HammerBuildConfiguration*
HammerBuildConfigurationFactory::restore(ProjectExplorer::Target* parent,
                                         const QVariantMap& map)
{
   if (!canRestore(parent, map))
       return 0;

   HammerBuildConfiguration *bc(new HammerBuildConfiguration(parent));
   if (bc->fromMap(map))
       return bc;

   delete bc;

   return 0;
}

BuildConfiguration::BuildType HammerBuildConfiguration::buildType() const
{
    return Unknown;
}

HammerBuildSettingsWidget::HammerBuildSettingsWidget(HammerBuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setHistoryCompleter(QLatin1String("Generic.BuildDir.History"));
    m_pathChooser->setEnabled(true);
    fl->addRow(tr("Build directory:"), m_pathChooser);
//    connect(m_pathChooser, &Utils::PathChooser::changed,
//            this, &HammerBuildSettingsWidget::buildDirectoryChanged);

    m_buildConfiguration = bc;
    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setEnvironment(bc->environment());
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    setDisplayName(tr("Hammer"));

    connect(bc, &HammerBuildConfiguration::environmentChanged,
            this, &HammerBuildSettingsWidget::environmentHasChanged);
}

void HammerBuildSettingsWidget::buildDirectoryChanged()
{
//    m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_pathChooser->rawPath()));
}

void HammerBuildSettingsWidget::environmentHasChanged()
{
    m_pathChooser->setEnvironment(m_buildConfiguration->environment());
}

}}
