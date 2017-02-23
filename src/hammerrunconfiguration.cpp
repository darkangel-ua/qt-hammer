#include <projectexplorer/target.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <hammer/core/main_target.h>
#include <hammer/core/collect_nodes.h>
#include <hammer/core/target_type.h>
#include <hammer/core/types.h>
#include <hammer/core/engine.h>

#include "hammerrunconfiguration.h"
#include "hammerrunconfigurationwidget.h"
#include "hammerproject.h"

static const char* const HAMMER_RUN_CONFIGURATION_ID = "HummerProjectManager.HammerRunConfiguration";
static const char* const COMMAND_ARGUMENTS_KEY = "HummerProjectManager.HummerRunConfiguration.CommandArguments";
static const char* const WORKING_DIRECTORY_KEY = "HummerProjectManager.HummerRunConfiguration.WorkingDirectory";

using namespace ProjectExplorer;

namespace hammer{ namespace QtCreator{

HammerRunConfiguration::HammerRunConfiguration(ProjectExplorer::Target* parent)
   : LocalApplicationRunConfiguration(parent, Core::Id(HAMMER_RUN_CONFIGURATION_ID)),
     m_target(parent)
{
   addExtraAspect(new LocalEnvironmentAspect(this));
   addExtraAspect(new ArgumentsAspect(this, QStringLiteral("HammerProjectManager.HammerRunConfiguration.Arguments")));
   addExtraAspect(new TerminalAspect(this, QStringLiteral("HammerProjectManager.HammerRunConfiguration.UseTerminal")));
   setDefaultDisplayName("Hammer Run");
}

HammerRunConfiguration::HammerRunConfiguration(ProjectExplorer::Target* parent,
                                               HammerRunConfiguration *source)
   : LocalApplicationRunConfiguration(parent, source),
     m_target(parent)
{
}

QString HammerRunConfiguration::executable() const
{
   if (m_executable)
      return *m_executable;

   const main_target& mt = qobject_cast<HammerProject*>(m_target->project())->get_main_target();
   try {
      build_nodes_t nodes = mt.generate();
      for (const build_node_ptr& node : nodes)
         for (const basic_target* bt : node->products_)
            if (bt->type().equal_or_derived_from(types::EXE)) {
               location_t l = bt->location() / bt->name();
               m_executable = QString::fromStdString(l.string());
               return *m_executable;
            } else if (bt->type().equal_or_derived_from(types::TESTING_OUTPUT)) {
               for (const build_node::source_t& src : node->sources_)
                  if (src.source_target_->type().equal_or_derived_from(types::EXE)) {
                     location_t l = src.source_target_->location() / src.source_target_->name();
                     m_executable = QString::fromStdString(l.string());
                     return *m_executable;
                  }
            }
   } catch(...) { }

   return QString();
}

ProjectExplorer::ApplicationLauncher::Mode
HammerRunConfiguration::runMode() const
{
   return extraAspect<TerminalAspect>()->runMode();
}

QString HammerRunConfiguration::workingDirectory() const
{
   return m_workingDirectory;
}

QString HammerRunConfiguration::commandLineArguments() const
{
   return extraAspect<ArgumentsAspect>()->arguments();
}

QWidget* HammerRunConfiguration::createConfigurationWidget()
{
   return new HammerRunConfigurationWidget(this);
}

void HammerRunConfiguration::setBaseWorkingDirectory(const QString& workingDirectory)
{
    m_workingDirectory = workingDirectory;
    emit baseWorkingDirectoryChanged(m_workingDirectory);
}

QString HammerRunConfiguration::baseWorkingDirectory() const
{
    return m_workingDirectory;
}

bool HammerRunConfiguration::fromMap(const QVariantMap &map)
{
   m_workingDirectory = map.value(WORKING_DIRECTORY_KEY).toString();

   return ProjectExplorer::LocalApplicationRunConfiguration::fromMap(map);
}

QVariantMap HammerRunConfiguration::toMap() const
{
   QVariantMap result(ProjectExplorer::LocalApplicationRunConfiguration::toMap());

   result.insert(WORKING_DIRECTORY_KEY, m_workingDirectory);

   return result;
}

/*
Utils::Environment
HammerRunConfiguration::environment() const
{
   if (!m_additionalPaths)
   {
      const main_target& mt = m_target->hammerProject()->get_main_target();

      try
      {
         typedef std::set<const build_node*> visited_nodes_t;

         build_nodes_t nodes = mt.generate();

         build_node::sources_t collected_nodes;
         visited_nodes_t visited_nodes;
         std::vector<const target_type*> types(1, &mt.get_engine()->get_type_registry().get(types::SHARED_LIB));
         collect_nodes(collected_nodes, visited_nodes, nodes, types, true);

         QStringList paths;
         BOOST_FOREACH(const build_node::source_t& s, collected_nodes)
            paths << QString::fromStdString(s.source_target_->location().string());

         paths.removeDuplicates();
         m_additionalPaths = paths;

      }catch(...) { exit(-1); m_additionalPaths = QStringList(); }
   }

   Utils::Environment env = CustomExecutableRunConfiguration::environment();

   BOOST_FOREACH(const QString& s, *m_additionalPaths)
      env.appendOrSetPath(s);

#if !defined(_WIN32)
   QString sep(":");
   BOOST_FOREACH(const QString& s, *m_additionalPaths)
      env.appendOrSet("LD_LIBRARY_PATH", s, sep);
#endif

   return env;
}

*/

// from projectexplorer/projectconfiguration.cpp
static const char * const CONFIGURATION_ID_KEY("ProjectExplorer.ProjectConfiguration.Id");

HammerRunConfigurationFactory::HammerRunConfigurationFactory(QObject *parent)
   : ProjectExplorer::IRunConfigurationFactory(parent)
{
}

QList<Core::Id>
HammerRunConfigurationFactory::availableCreationIds(ProjectExplorer::Target* parent,
                                                    CreationMode mode) const
{
   if (qobject_cast<HammerProject*>(parent->project()))
      return { HAMMER_RUN_CONFIGURATION_ID };
   else
      return {};
}

QString HammerRunConfigurationFactory::displayNameForId(Core::Id id) const
{
   return tr("Hammer Executable");
}

bool HammerRunConfigurationFactory::canCreate(ProjectExplorer::Target* parent,
                                              Core::Id id) const
{
   return qobject_cast<HammerProject*>(parent->project()) && id == Core::Id(HAMMER_RUN_CONFIGURATION_ID);
}

bool HammerRunConfigurationFactory::canRestore(ProjectExplorer::Target* parent,
                                               const QVariantMap& map) const
{
   return canCreate(parent, ProjectExplorer::idFromMap(map));
}

bool HammerRunConfigurationFactory::canClone(ProjectExplorer::Target *parent,
                                             ProjectExplorer::RunConfiguration *source) const
{
   return canCreate(parent, source->id());
}

ProjectExplorer::RunConfiguration*
HammerRunConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                     ProjectExplorer::RunConfiguration *source)
{
   if (!canClone(parent, source))
       return nullptr;

   return new HammerRunConfiguration(parent, static_cast<HammerRunConfiguration*>(source));
}

ProjectExplorer::RunConfiguration*
HammerRunConfigurationFactory::doCreate(ProjectExplorer::Target* parent,
                                        Core::Id id)
{
   return new HammerRunConfiguration(parent);
}

ProjectExplorer::RunConfiguration*
HammerRunConfigurationFactory::doRestore(ProjectExplorer::Target* parent,
                                         const QVariantMap& map)
{
   return new HammerRunConfiguration(parent);
}

}}
