#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>
#include <boost/filesystem/operations.hpp>

#include <hammer/core/toolset_manager.h>
#include <hammer/core/toolsets/gcc_toolset.h>
#include <hammer/core/toolsets/msvc_toolset.h>
#include <hammer/core/toolsets/qt_toolset.h>
#include <hammer/core/htmpl/htmpl.h>
#include <hammer/core/warehouse.h>
#include <hammer/core/types.h>
#include <hammer/core/generator_registry.h>
#include <hammer/core/copy_generator.h>
#include <hammer/core/obj_generator.h>
#include <hammer/core/testing_generators.h>
#include <hammer/core/header_lib_generator.h>
#include <hammer/core/c_scanner.h>
#include <hammer/core/scaner_manager.h>
#include <hammer/core/feature_set.h>
#include <hammer/core/main_target.h>
#include <hammer/core/target_type.h>
#include <hammer/core/fs_helpers.h>
#include <hammer/core/feature.h>
#include <hammer/core/subfeature.h>

#include <QFileInfo>
#include <QAction>
#include <QMessageBox>
#include <QMenu>

#include "hammerprojectmanager.h"
#include "hammerproject.h"
#include "hammerprojectnode.h"
#include "hammerprojectconstants.h"

using namespace std;
namespace fs = boost::filesystem;

namespace hammer{ namespace QtCreator{

static
void use_toolset_rule(project*, engine& e, string& toolset_name, string& toolset_version, string* toolset_home_)
{
   location_t toolset_home;
   if (toolset_home_ != NULL)
      toolset_home = *toolset_home_;

   e.toolset_manager().init_toolset(e, toolset_name, toolset_version, toolset_home_ == NULL ? NULL : &toolset_home);
}

static
hammer::location_t
get_user_config_location()
{
#if defined(_WIN32)
   const char* home_path = getenv("USERPROFILE");
   if (home_path != NULL)
      return hammer::location_t(home_path) / "user-config.ham";
   else
      throw std::runtime_error("Can't find user home directory.");

#else
#   if defined(__linux__)

   const char* home_path = getenv("HOME");
   if (home_path != NULL)
      return hammer::location_t(home_path) / "user-config.ham";
   else
      throw std::runtime_error("Can't find user home directory.");

#   else
#      error "Platform not supported"
#   endif
#endif
}

static
std::unique_ptr<engine>
construct_default_engine()
{
   std::unique_ptr<engine> default_engine(new engine);

   install_warehouse_rules(default_engine->call_resolver(), *default_engine);
   types::register_standart_types(default_engine->get_type_registry(), default_engine->feature_registry());
   default_engine->generators().insert(std::auto_ptr<generator>(new copy_generator(*default_engine)));
   default_engine->generators().insert(std::auto_ptr<generator>(new obj_generator(*default_engine)));
   add_testing_generators(*default_engine, default_engine->generators());
   add_header_lib_generator(*default_engine, default_engine->generators());
   install_htmpl(*default_engine);

   boost::shared_ptr<scanner> c_scaner(new hammer::c_scanner);
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::CPP), c_scaner);
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::C), c_scaner);
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::RC), c_scaner);

   default_engine->toolset_manager().add_toolset(auto_ptr<toolset>(new msvc_toolset));
   default_engine->toolset_manager().add_toolset(auto_ptr<toolset>(new gcc_toolset));
   default_engine->toolset_manager().add_toolset(auto_ptr<toolset>(new qt_toolset));

   default_engine->call_resolver().insert("use-toolset", boost::function<void (project*, string&, string&, string*)>(boost::bind(use_toolset_rule, _1, boost::ref(*default_engine), _2, _3, _4)));

   const location_t user_config_script = get_user_config_location();
   if (!user_config_script.empty() && exists(user_config_script))
      default_engine->load_hammer_script(user_config_script);

   default_engine->toolset_manager().autoconfigure(*default_engine);

   return default_engine;
}

ProjectManager::ProjectManager()
{
   Core::ActionContainer* tools_container = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);

   reload_action_ = new QAction(tr("Reload all projects"), this);
   Core::ActionContainer* hammer_main_menu = Core::ActionManager::createMenu("Hammer.MainMenu");
   hammer_main_menu->menu()->setTitle("Hammer");

   Core::Command* reload_command = Core::ActionManager::registerAction(reload_action_, "Hammer.ReloadProjects");
   reload_command->setDefaultKeySequence(QKeySequence(tr("Meta+H,R")));
   connect(reload_action_, &QAction::triggered, [this]{ on_reload(); });
   hammer_main_menu->addAction(reload_command);

   tools_container->addMenu(hammer_main_menu);

   engine_ = construct_default_engine();
}

ProjectManager::~ProjectManager()
{
}

QString ProjectManager::mimeType() const
{
   return QLatin1String(HAMMERMIMETYPE);
}

void gatherAllMainTargets(boost::unordered_set<const main_target*>& targets,
                          const main_target& targetToInspect)
{
   if (targets.find(&targetToInspect) != targets.end())
      return;

   targets.insert(&targetToInspect);

   BOOST_FOREACH(const basic_target* bt, targetToInspect.sources())
      gatherAllMainTargets(targets, *bt->get_main_target());

   BOOST_FOREACH(const basic_target* bt, targetToInspect.dependencies())
      gatherAllMainTargets(targets, *bt->get_main_target());
}

static
const main_target&
instantiate_project(hammer::engine& e,
                    const hammer::project& p)
{
   // find out which target to build
   const basic_meta_target* target = NULL;
   for (hammer::project::targets_t::const_iterator i = p.targets().begin(), last = p.targets().end(); i != last; ++i) {
      if (!i->second->is_explicit()) {
         if (target != NULL)
            throw std::runtime_error("Project contains more than one implicit target");
         else
            target = i->second;
      }
   }

   // create environment for instantiation
   feature_set* build_request = e.feature_registry().make_set();

   // lets handle 'toolset' feature in build request
#if defined(_WIN32)
   const string default_toolset_name = "msvc";
#else
   const string default_toolset_name = "gcc";
#endif
   auto i_toolset_in_build_request = build_request->find("toolset");
   if (i_toolset_in_build_request == build_request->end()) {
      const feature_def& toolset_definition = e.feature_registry().get_def("toolset");
      if (!toolset_definition.is_legal_value(default_toolset_name))
         throw std::runtime_error("Default toolset is set to '"+ default_toolset_name + "', but either you didn't configure it in user-config.ham or it has failed to autoconfigure");

      const subfeature_def& toolset_version_def = toolset_definition.get_subfeature("version");
      if (toolset_version_def.legal_values(default_toolset_name).size() == 1)
         build_request->join("toolset", (default_toolset_name + "-" + *toolset_version_def.legal_values(default_toolset_name).begin()).c_str());
      else
         throw std::runtime_error("Default toolset is set to '"+ default_toolset_name + "', but has multiple version configured. You should request specific version to use.");
   } else {
      const feature& used_toolset = **i_toolset_in_build_request;
      if (!used_toolset.find_subfeature("version")) {
         const subfeature_def& toolset_version_def = used_toolset.definition().get_subfeature("version");
         if (toolset_version_def.legal_values(used_toolset.value()).size() > 1)
            throw std::runtime_error("Toolset is set to '"+ used_toolset.value() + "', but has multiple version configured. You should request specific version to use.");
         else {
            const string toolset = used_toolset.value();
            build_request->erase_all("toolset");
            build_request->join("toolset", (toolset + "-" + *toolset_version_def.legal_values(toolset).begin()).c_str());
         }
      }
   }

   if (build_request->find("variant") == build_request->end())
      build_request->join("variant", "debug");

   if (build_request->find("host-os") == build_request->end())
      build_request->join("host-os", e.feature_registry().get_def("host-os").get_default().c_str());

   // instantiate selected target
   vector<basic_target*> instantiated_targets;
   feature_set* usage_requirements = e.feature_registry().make_set();
   target->instantiate(NULL, *build_request, &instantiated_targets, usage_requirements);

   if (instantiated_targets.size() != 1)
      throw std::runtime_error("Target instantiation produce more than one result");

   return *dynamic_cast<main_target*>(instantiated_targets[0]);
}

static
const hammer::main_target&
load_project(hammer::engine& e,
             const std::string& hamfile_path)
{
   const QString q_hamfile_path = QDir::toNativeSeparators(QString::fromStdString(hamfile_path));
   const hammer::location_t project_dir = resolve_symlinks(hamfile_path).branch_path();
   const hammer::project& newly_loaded_hammer_project = [&]() -> hammer::project& {
      try {
         return e.load_project(project_dir);
      } catch (const std::exception& e) {
         throw std::runtime_error(QString("Failed to load project '%1': %2").arg(q_hamfile_path).arg(e.what()).toStdString());
      }
   }();

   return instantiate_project(e, newly_loaded_hammer_project);
}

ProjectExplorer::Project*
ProjectManager::openProject(const QString& fileName,
                            QString* errorString)
{
   if (!QFileInfo(fileName).isFile())
        return NULL;

    foreach (ProjectExplorer::Project *pi, ProjectExplorer::SessionManager::instance()->projects()) {
        if (fileName == pi->document()->filePath().toString()) {
            Core::MessageManager::write(tr("Failed opening project '%1': Project already open").arg(QDir::toNativeSeparators(fileName)),
                                        Core::MessageManager::WithFocus);
            return NULL;
        }
    }

    try {
       const hammer::main_target& mt = load_project(*engine_, fileName.toStdString());
       HammerProject* mainProject = new HammerProject(this, &mt, /*main_project=*/true);

       return mainProject;
    } catch (const std::exception& e) {
       Core::MessageManager::write(tr("Failed open project '%1': %2").arg(QDir::toNativeSeparators(fileName)).arg(e.what()),
                                   Core::MessageManager::WithFocus);
       return NULL;
    }
}

void ProjectManager::on_reload()
{
   const auto all_projects = ProjectExplorer::SessionManager::projects();

   std::unique_ptr<engine> new_engine = construct_default_engine();
   vector<pair<HammerProject*, const hammer::main_target*> > reloaded_data;
   for (ProjectExplorer::Project* p : all_projects) {
      if (HammerProject* hammer_project = dynamic_cast<HammerProject*>(p)) {
         const Utils::FileName& hamfile_path = hammer_project->document()->filePath();
         const hammer::main_target& mt = load_project(*new_engine, hamfile_path.toString().toStdString());
         reloaded_data.push_back({hammer_project, &mt});
      }
   }

   for (const auto& p : reloaded_data)
      p.first->reload(p.second);

   std::swap(engine_, new_engine);
}

}}
