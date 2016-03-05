#include "hammermakestep.h"
#include "hammerprojectconstants.h"
#include "hammerproject.h"
#include "hammerbuildconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/gnumakeparser.h>
//#include <coreplugin/variablemanager.h>
#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <QCoreApplication>
#include <QListWidgetItem>
#include "hammer_make_step_config_widget.h"

namespace {
   const char * const HAMMER_MS_ID("HammerProjectManager.HammerMakeStep");
   const char * const HAMMER_MS_DISPLAY_NAME(QT_TRANSLATE_NOOP("HammerProjectManager::Internal::HammerMakeStep", "Hammer"));
   const char * const HAMMER_MAKE_CURRENT_DISPLAY_NAME(QT_TRANSLATE_NOOP("HammerProjectManager::Internal::HammerMakeCurrentStep", "MakeCurrent"));

   const char * const BUILD_TARGETS_KEY("HammerProjectManager.HammerMakeStep.BuildTargets");
   const char * const MAKE_ARGUMENTS_KEY("HammerProjectManager.HammerMakeStep.MakeArguments");
   const char * const MAKE_COMMAND_KEY("HammerProjectManager.HammerMakeStep.MakeCommand");
}

namespace hammer{ namespace QtCreator{

HammerMakeStep::HammerMakeStep(ProjectExplorer::BuildStepList* parent) :
AbstractProcessStep(parent, Core::Id(HAMMER_MS_ID))
{
   ctor();
}

HammerMakeStep::HammerMakeStep(ProjectExplorer::BuildStepList* parent,
                               HammerMakeStep* bs)
   : AbstractProcessStep(parent, bs),
     m_buildTargets(bs->m_buildTargets),
     m_makeArguments(bs->m_makeArguments),
     m_makeCommand(bs->m_makeCommand)
{
   ctor();
}

void HammerMakeStep::ctor()
{
   setDefaultDisplayName(QCoreApplication::translate("HammerProjectManager::Internal::HammerMakeStep", HAMMER_MS_DISPLAY_NAME));
   m_makeCommand = "dhammer";
}

HammerMakeStep::~HammerMakeStep()
{
}

HammerBuildConfiguration*
HammerMakeStep::hammerBuildConfiguration() const
{
   return static_cast<HammerBuildConfiguration*>(buildConfiguration());
}

bool HammerMakeStep::init()
{
   HammerBuildConfiguration* bc = hammerBuildConfiguration();

   setEnabled(true);
   ProjectExplorer::ProcessParameters* pp = processParameters();
   pp->setMacroExpander(bc->macroExpander());
   pp->setWorkingDirectory(bc->buildDirectory().toString());
   pp->setEnvironment(bc->environment());
   pp->setCommand(makeCommand());
   pp->setArguments(allArguments());

   setOutputParser(new ProjectExplorer::GnuMakeParser());
   ProjectExplorer::IOutputParser* parser = target()->kit()->createOutputParser();
   if (parser)
       appendOutputParser(parser);
   outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

   return AbstractProcessStep::init();}

QVariantMap HammerMakeStep::toMap() const
{
   QVariantMap map(AbstractProcessStep::toMap());

   map.insert(QLatin1String(BUILD_TARGETS_KEY), m_buildTargets);
   map.insert(QLatin1String(MAKE_ARGUMENTS_KEY), m_makeArguments);
   map.insert(QLatin1String(MAKE_COMMAND_KEY), m_makeCommand);
   return map;
}

bool HammerMakeStep::fromMap(const QVariantMap &map)
{
   m_buildTargets = map.value(QLatin1String(BUILD_TARGETS_KEY)).toStringList();
   m_makeArguments = map.value(QLatin1String(MAKE_ARGUMENTS_KEY)).toString();
   m_makeCommand = map.value(QLatin1String(MAKE_COMMAND_KEY)).toString();

   return BuildStep::fromMap(map);
}

QString HammerMakeStep::allArguments() const
{
   QString args = m_makeArguments;
   Utils::QtcProcess::addArgs(&args, m_buildTargets);
   return args;
}

QString HammerMakeStep::makeCommand() const
{
   return m_makeCommand;
}

void HammerMakeStep::run(QFutureInterface<bool> &fi)
{
   AbstractProcessStep::run(fi);
}

ProjectExplorer::BuildStepConfigWidget *HammerMakeStep::createConfigWidget()
{
   return new hammer_make_step_config_widget(this);
}

bool HammerMakeStep::immutable() const
{
   return false;
}

bool HammerMakeStep::buildsTarget(const QString &target) const
{
   return m_buildTargets.contains(target);
}

void HammerMakeStep::setBuildTarget(const QString &target, bool on)
{
   QStringList old = m_buildTargets;
   if (on && !old.contains(target))
      old << target;
   else if(!on && old.contains(target))
      old.removeOne(target);

   m_buildTargets = old;
}

HammerMakeCurrentStep::HammerMakeCurrentStep(ProjectExplorer::BuildStepList *parent)
   : AbstractProcessStep(parent, Core::Id(HAMMER_MAKE_CURRENT_ID))
{

}

HammerMakeCurrentStep::HammerMakeCurrentStep(ProjectExplorer::BuildStepList *parent, HammerMakeCurrentStep *bs)
   : AbstractProcessStep(parent, bs)
{

}

ProjectExplorer::BuildStepConfigWidget*
HammerMakeCurrentStep::createConfigWidget()
{
   return NULL;
}

bool HammerMakeCurrentStep::init()
{
   HammerBuildConfiguration *bc = hammerBuildConfiguration();

   setEnabled(true);
   ProjectExplorer::ProcessParameters *pp = processParameters();
   pp->setMacroExpander(bc->macroExpander());
   pp->setWorkingDirectory(bc->buildDirectory().toString());
   pp->setEnvironment(bc->environment());
   pp->setCommand("hammer");

   setOutputParser(new ProjectExplorer::GnuMakeParser());
   outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

   return AbstractProcessStep::init();
}

void HammerMakeCurrentStep::setTargetToBuid(const QString& target, const QString& projectPath)
{
   ProjectExplorer::ProcessParameters *pp = processParameters();
   pp->setArguments("--just-one-source \"" + target + "\" --just-one-source-project-path \"" + projectPath + "\"");
}

HammerBuildConfiguration *HammerMakeCurrentStep::hammerBuildConfiguration() const
{
   return static_cast<HammerBuildConfiguration *>(buildConfiguration());
}

HammerMakeStepFactory::HammerMakeStepFactory(QObject *parent)
   : ProjectExplorer::IBuildStepFactory(parent)
{
}

HammerMakeStepFactory::~HammerMakeStepFactory()
{
}

bool HammerMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent,
                                      Core::Id id) const
{
   if (parent->target()->project()->id() != Core::Id(HAMMERPROJECT_ID))
      return false;

   return id == Core::Id(HAMMER_MS_ID) ||
          id == Core::Id(HAMMER_MAKE_CURRENT_ID);
}

ProjectExplorer::BuildStep *HammerMakeStepFactory::create(ProjectExplorer::BuildStepList *parent,
                                                          const Core::Id id)
{
   if (!canCreate(parent, id))
      return NULL;

   if (id == Core::Id(HAMMER_MAKE_CURRENT_ID))
      return new HammerMakeCurrentStep(parent);
   else
      return new HammerMakeStep(parent);
}

bool HammerMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent,
                                     ProjectExplorer::BuildStep *source) const
{
   return canCreate(parent, source->id());
}

ProjectExplorer::BuildStep *HammerMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent,
                                                         ProjectExplorer::BuildStep *source)
{
   if (!canClone(parent, source))
      return NULL;

   if (HammerMakeStep* old = qobject_cast<HammerMakeStep*>(source))
      return new HammerMakeStep(parent, old);
   else
      if (HammerMakeCurrentStep* old = qobject_cast<HammerMakeCurrentStep*>(source))
         return new HammerMakeCurrentStep(parent, old);
      else
         return NULL;
}

bool HammerMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent,
                                        const QVariantMap &map) const
{
   return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *HammerMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent,
                                                           const QVariantMap &map)
{
   if (!canRestore(parent, map))
      return NULL;

   ProjectExplorer::BuildStep* bs = create(parent, ProjectExplorer::idFromMap(map));
   if (!bs)
      return NULL;

   if (bs->fromMap(map))
      return bs;

   delete bs;
   return NULL;
}

QList<Core::Id>
HammerMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList* parent) const
{
   if (parent->target()->project()->id() != Core::Id(HAMMERPROJECT_ID))
      return {};

   return {Core::Id(HAMMER_MS_ID), Core::Id(HAMMER_MAKE_CURRENT_ID)};
}

QString HammerMakeStepFactory::displayNameForId(Core::Id id) const
{
   if (id == Core::Id(HAMMER_MS_ID))
      return QCoreApplication::translate("HammerProjectManager::Internal::HammerMakeStep", HAMMER_MS_DISPLAY_NAME);

   return QString();
}

}}
