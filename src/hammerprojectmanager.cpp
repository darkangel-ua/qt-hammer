#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>

#include <boost/bind.hpp>
#include <boost/unordered_set.hpp>
#include <boost/filesystem/operations.hpp>

#include <hammer/core/toolset_manager.h>
#include <hammer/core/toolsets/gcc_toolset.h>
#include <hammer/core/toolsets/msvc_toolset.h>
#include <hammer/core/toolsets/qt_toolset.h>
#include <hammer/core/htmpl/htmpl.h>
#include <hammer/core/warehouse.h>
#include <hammer/core/warehouse_target.h>
#include <hammer/core/types.h>
#include <hammer/core/generator_registry.h>
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
#include <hammer/core/warehouse.h>
#include <hammer/core/warehouse_manager.h>
#include <hammer/core/instantiation_context.h>
#include <hammer/core/system_paths.h>

#include <QFileInfo>
#include <QAction>
#include <QMessageBox>
#include <QMenu>

#include "hammerprojectmanager.h"
#include "hammerproject.h"
#include "hammerprojectnode.h"
#include "hammerprojectconstants.h"
#include "download_packages_dialog.h"

using namespace std;
namespace fs = boost::filesystem;

namespace hammer{ namespace QtCreator{

static
std::unique_ptr<engine>
construct_default_engine()
{
   std::unique_ptr<engine> default_engine(new engine);

   install_warehouse_rules(*default_engine);
   types::register_standart_types(default_engine->get_type_registry(), default_engine->feature_registry());
   default_engine->generators().insert(std::unique_ptr<generator>(new obj_generator(*default_engine)));
   add_testing_generators(*default_engine, default_engine->generators());
   add_header_lib_generator(*default_engine, default_engine->generators());
   install_htmpl(*default_engine);

   auto c_scaner = std::make_shared<hammer::c_scanner>();
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::CPP), c_scaner);
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::C), c_scaner);
   default_engine->scanner_manager().register_scanner(default_engine->get_type_registry().get(types::RC), c_scaner);

   default_engine->toolset_manager().add_toolset(*default_engine, unique_ptr<toolset>(new msvc_toolset));
   default_engine->toolset_manager().add_toolset(*default_engine, unique_ptr<toolset>(new gcc_toolset));
   default_engine->toolset_manager().add_toolset(*default_engine, unique_ptr<toolset>(new qt_toolset));

   const location_t user_config_script = get_system_paths().config_file_;
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

   for(const basic_target* bt : targetToInspect.sources())
      gatherAllMainTargets(targets, *bt->get_main_target());

   for(const basic_target* bt : targetToInspect.dependencies())
      gatherAllMainTargets(targets, *bt->get_main_target());
}

static
const main_target&
instantiate_project(hammer::engine& e,
                    const hammer::project& p)
{
   // find out which target to build
   const basic_meta_target* target = nullptr;
   for (hammer::project::targets_t::const_iterator i = p.targets().begin(), last = p.targets().end(); i != last; ++i) {
      if (!i->second->is_explicit() && !i->second->is_local()) {
         if (target)
            throw std::runtime_error("Project contains more than one implicit target");
         else
            target = i->second.get();
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
      // peek first configured as default
      const string& default_toolset_version = *toolset_version_def.legal_values(default_toolset_name).begin();
      build_request->join("toolset", (default_toolset_name + "-" + default_toolset_version).c_str());
   } else {
      const feature& used_toolset = **i_toolset_in_build_request;
      if (!used_toolset.find_subfeature("version")) {
         const subfeature_def& toolset_version_def = used_toolset.definition().get_subfeature("version");
         const string& toolset = used_toolset.value();
         // peek first configured as default
         const string& default_toolset_version = *toolset_version_def.legal_values(toolset).begin();
         build_request->erase_all("toolset");
         build_request->join("toolset", (toolset + "-" + default_toolset_version).c_str());
      }
   }

   if (build_request->find("variant") == build_request->end())
      build_request->join("variant", "debug");

   if (build_request->find("host-os") == build_request->end())
      build_request->join("host-os", e.feature_registry().get_def("host-os").get_defaults().front().value_.c_str());

   if (build_request->find("target-os") == build_request->end())
      build_request->join("target-os", e.feature_registry().get_def("target-os").get_defaults().front().value_.c_str());

   // instantiate selected target
   vector<basic_target*> instantiated_targets;
   feature_set* usage_requirements = e.feature_registry().make_set();
   instantiation_context ctx;
   target->instantiate(ctx, nullptr, *build_request, &instantiated_targets, usage_requirements);

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
   const hammer::project& newly_loaded_hammer_project = [&]() -> const hammer::project& {
      try {
         return e.load_project(project_dir);
      } catch (const std::exception& e) {
         throw std::runtime_error(QString("Failed to load project '%1': %2").arg(q_hamfile_path).arg(e.what()).toStdString());
      }
   }();

   return instantiate_project(e, newly_loaded_hammer_project);
}

struct ProjectManager::cloned_state
{
   std::unique_ptr<engine> engine_ = construct_default_engine();
   vector<pair<HammerProject*, const hammer::main_target*> > projects_data_;
};

ProjectManager::cloned_state
clone_current_state()
{
   const auto all_projects = ProjectExplorer::SessionManager::projects();

   ProjectManager::cloned_state cloned_state;
   for (ProjectExplorer::Project* p : all_projects) {
      if (HammerProject* hammer_project = dynamic_cast<HammerProject*>(p)) {
         const Utils::FileName& hamfile_path = hammer_project->document()->filePath();
         const hammer::main_target& mt = load_project(*cloned_state.engine_, hamfile_path.toString().toStdString());
         cloned_state.projects_data_.push_back({hammer_project, &mt});
      }
   }

   return cloned_state;
}

ProjectExplorer::Project*
ProjectManager::openProject(const QString& fileName,
                            QString* errorString)
{
   if (!QFileInfo(fileName).isFile())
      return nullptr;

   for(ProjectExplorer::Project *pi : ProjectExplorer::SessionManager::instance()->projects()) {
      if (fileName == pi->document()->filePath().toString()) {
         Core::MessageManager::write(tr("Failed opening project '%1': Project already open").arg(QDir::toNativeSeparators(fileName)),
                                     Core::MessageManager::WithFocus);
         return nullptr;
      }
   }

   try {
      cloned_state cloned_state = clone_current_state();
      const hammer::main_target& mt = load_project(*cloned_state.engine_, fileName.toStdString());

      warehouse& wh = *cloned_state.engine_->warehouse_manager().get_default();
      const auto unresolved_targets = find_all_warehouse_unresolved_targets({const_cast<main_target*>(&mt)});
      if (!unresolved_targets.empty()) {
         vector<warehouse::package_info> packages = wh.get_unresoved_targets_info(*cloned_state.engine_, unresolved_targets);
         sort(packages.begin(), packages.end(), [](const warehouse::package_info& lhs, const warehouse::package_info& rhs) { return lhs.name_ < rhs.name_; });
         download_packages_dialog dlg(nullptr, *cloned_state.engine_, packages);
         if (dlg.exec() != QDialog::Accepted)
            return nullptr;
         else {
            // because we just added new warehouse targets we need to reload everything + this new project
            // we need to use new engine because upper cloned_state contains warehouse traps
            ProjectManager::cloned_state cloned_state = clone_current_state();
            const hammer::main_target& mt = load_project(*cloned_state.engine_, fileName.toStdString());
            HammerProject* mainProject = new HammerProject(this, &mt, /*main_project=*/true);

            // I hope this will not throw otherwise bad things happens...
            reload(std::move(cloned_state));

            return mainProject;
         }
      } else {
         // at this point we sure that we can load project without errors and unresolved warehouse targets
         // except cases when some UFO modified hamfiles in-between :(
         const hammer::main_target& mt = load_project(*engine_, fileName.toStdString());
         HammerProject* mainProject = new HammerProject(this, &mt, /*main_project=*/true);

         return mainProject;
      }
   } catch (const std::exception& e) {
      Core::MessageManager::write(tr("Failed open project '%1': %2").arg(QDir::toNativeSeparators(fileName)).arg(e.what()),
                                  Core::MessageManager::WithFocus);
      return nullptr;
   }
}

void ProjectManager::on_reload()
{
   reload(clone_current_state());
}

void ProjectManager::reload(cloned_state state)
{
   for (const auto& p : state.projects_data_)
      p.first->reload(p.second);

   std::swap(engine_, state.engine_);
}

}}
