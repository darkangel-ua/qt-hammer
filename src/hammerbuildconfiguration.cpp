#include "hammerbuildconfiguration.h"

#include "hammermakestep.h"
#include "hammerproject.h"
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

using namespace ProjectExplorer;

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
   return nullptr;
}

HammerBuildConfigurationFactory::HammerBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

HammerBuildConfigurationFactory::~HammerBuildConfigurationFactory()
{
}

QList<ProjectExplorer::BuildInfo*>
HammerBuildConfigurationFactory::createBuildInfo(const ProjectExplorer::Kit* k,
                                                 const Utils::FileName& buildDir) const
{
   QList<ProjectExplorer::BuildInfo*> result;

   ProjectExplorer::BuildInfo* debug_info = new ProjectExplorer::BuildInfo(this);
   debug_info->typeName = tr("Debug");
   debug_info->displayName = tr("Debug");
   debug_info->buildDirectory = buildDir;
   debug_info->kitId = k->id();
   debug_info->buildType = BuildConfiguration::Debug;

   ProjectExplorer::BuildInfo* release_info = new ProjectExplorer::BuildInfo(this);
   release_info->typeName = tr("Release");
   release_info->displayName = tr("Release");
   release_info->buildDirectory = buildDir;
   release_info->kitId = k->id();
   release_info->buildType = BuildConfiguration::Release;

   ProjectExplorer::BuildInfo* profile_info = new ProjectExplorer::BuildInfo(this);
   profile_info->typeName = tr("Profile");
   profile_info->displayName = tr("Profile");
   profile_info->buildDirectory = buildDir;
   profile_info->kitId = k->id();
   profile_info->buildType = BuildConfiguration::Profile;

   result << debug_info << release_info << profile_info;

   return result;
}

int HammerBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
   return canHandle(parent) ? 0 : -1;
}

QList<ProjectExplorer::BuildInfo*>
HammerBuildConfigurationFactory::availableBuilds(const ProjectExplorer::Target* parent) const
{
   return createBuildInfo(parent->kit(), parent->project()->projectDirectory());
}

int HammerBuildConfigurationFactory::priority(const ProjectExplorer::Kit* k,
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
   return createBuildInfo(k, Utils::FileName::fromString(QDir(projectPath).path()));
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
//   bc->setBuildDirectory(info->buildDirectory);
   bc->build_type_ = info->buildType;

   ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
//   ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

   Q_ASSERT(buildSteps);
   HammerMakeStep *makeStep = new HammerMakeStep(buildSteps);
   buildSteps->insertStep(0, makeStep);

   switch(bc->build_type_) {
      case BuildConfiguration::Debug:
         makeStep->set_arguments(QLatin1String("variant=debug"));
         break;

      case BuildConfiguration::Release:
         makeStep->set_arguments(QLatin1String("variant=release"));
         break;

      case BuildConfiguration::Profile:
         makeStep->set_arguments(QLatin1String("variant=profile"));
         break;

      default:
         Q_ASSERT(false);
         return nullptr;
   }

//   Q_ASSERT(cleanSteps);
//   HammerMakeStep* cleanMakeStep = new HammerMakeStep(cleanSteps);
//   cleanSteps->insertStep(0, cleanMakeStep);
//   cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
//   cleanMakeStep->setClean(true);

   return bc;
}

// from QtCreator projectexplorer/buildconfiguration.cpp
static const char* const BUILD_STEP_LIST_COUNT("ProjectExplorer.BuildConfiguration.BuildStepListCount");
static const char* const BUILD_STEP_LIST_PREFIX("ProjectExplorer.BuildConfiguration.BuildStepList.");

bool HammerBuildConfigurationFactory::canHandle(const ProjectExplorer::Target* t) const
{
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

BuildConfiguration::BuildType
HammerBuildConfiguration::buildType() const
{
    return build_type_;
}

/*
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
*/

}}
