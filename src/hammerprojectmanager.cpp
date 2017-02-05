#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/icore.h>

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

#include "hammerprojectmanager.h"
#include "hammerproject.h"
#include "hammerprojectnode.h"
#include "hammerprojectconstants.h"

using namespace std;
namespace fs = boost::filesystem;

namespace hammer{ namespace QtCreator{

static
void use_toolset_rule(project*, engine& e, pstring& toolset_name, pstring& toolset_version, pstring* toolset_home_)
{
   location_t toolset_home;
   if (toolset_home_ != NULL)
      toolset_home = toolset_home_->to_string();

   e.toolset_manager().init_toolset(e, toolset_name.to_string(), toolset_version.to_string(), toolset_home_ == NULL ? NULL : &toolset_home);
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

ProjectManager::ProjectManager()
{
   install_warehouse_rules(m_engine.call_resolver(), m_engine);
   types::register_standart_types(m_engine.get_type_registry(), m_engine.feature_registry());
   m_engine.generators().insert(std::auto_ptr<generator>(new copy_generator(m_engine)));
   m_engine.generators().insert(std::auto_ptr<generator>(new obj_generator(m_engine)));
   add_testing_generators(m_engine, m_engine.generators());
   add_header_lib_generator(m_engine, m_engine.generators());
   install_htmpl(m_engine);

   boost::shared_ptr<scanner> c_scaner(new hammer::c_scanner);
   m_engine.scanner_manager().register_scanner(m_engine.get_type_registry().get(types::CPP), c_scaner);
   m_engine.scanner_manager().register_scanner(m_engine.get_type_registry().get(types::C), c_scaner);
   m_engine.scanner_manager().register_scanner(m_engine.get_type_registry().get(types::RC), c_scaner);

   m_engine.toolset_manager().add_toolset(auto_ptr<toolset>(new msvc_toolset));
   m_engine.toolset_manager().add_toolset(auto_ptr<toolset>(new gcc_toolset));
   m_engine.toolset_manager().add_toolset(auto_ptr<toolset>(new qt_toolset));

   m_engine.call_resolver().insert("use-toolset", boost::function<void (project*, pstring&, pstring&, pstring*)>(boost::bind(use_toolset_rule, _1, boost::ref(m_engine), _2, _3, _4)));

   const location_t user_config_script = get_user_config_location();
   if (!user_config_script.empty() && exists(user_config_script))
      m_engine.load_hammer_script(user_config_script);

   m_engine.toolset_manager().autoconfigure(m_engine);
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

    hammer::project* m_hammerMasterProject;
    // load hammer project
    try {
       m_hammerMasterProject = &m_engine.load_project(resolve_symlinks(location_t(fileName.toStdString())).branch_path());
    } catch(const std::exception& e) {
       Core::MessageManager::write(tr("Failed opening project '%1': %2").arg(QDir::toNativeSeparators(fileName)).arg(e.what()),
                                   Core::MessageManager::WithFocus);

       return NULL;
    }

    // find out which target to build
    const basic_meta_target* target = NULL;
    for(hammer::project::targets_t::const_iterator i = m_hammerMasterProject->targets().begin(), last = m_hammerMasterProject->targets().end(); i != last; ++i) {
       if (!i->second->is_explicit()) {
          if (target != NULL) {
             Core::MessageManager::write(tr("Failed opening project '%1': Project contains more than one implicit target").arg(QDir::toNativeSeparators(fileName)),
                                         Core::MessageManager::WithFocus);

             return NULL;
          } else
            target = i->second;
       }
    }

    // create environment for instantiation
    feature_set* build_request = m_engine.feature_registry().make_set();

    // lets handle 'toolset' feature in build request
#if defined(_WIN32)
    const string default_toolset_name = "msvc";
#else
    const string default_toolset_name = "gcc";
#endif
    auto i_toolset_in_build_request = build_request->find("toolset");
    try {
       if (i_toolset_in_build_request == build_request->end()) {
          const feature_def& toolset_definition = m_engine.feature_registry().get_def("toolset");
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
             if (toolset_version_def.legal_values(used_toolset.value().to_string()).size() > 1)
                throw std::runtime_error("Toolset is set to '"+ used_toolset.value().to_string() + "', but has multiple version configured. You should request specific version to use.");
             else {
                const string toolset = used_toolset.value().to_string();
                build_request->erase_all("toolset");
                build_request->join("toolset", (toolset + "-" + *toolset_version_def.legal_values(toolset).begin()).c_str());
             }
          }
       }
    } catch (const std::exception& e) {
       Core::MessageManager::write(tr("Failed opening project '%1': %2").arg(QDir::toNativeSeparators(fileName)).arg(e.what()),
                                   Core::MessageManager::WithFocus);

       return NULL;
    }

    if (build_request->find("variant") == build_request->end())
       build_request->join("variant", "debug");

    if (build_request->find("host-os") == build_request->end())
       build_request->join("host-os", m_engine.feature_registry().get_def("host-os").get_default().c_str());

    // instantiate selected target
    vector<basic_target*> instantiated_targets;
    try {
       feature_set* usage_requirements = m_engine.feature_registry().make_set();
       target->instantiate(NULL, *build_request, &instantiated_targets, usage_requirements);
    } catch(const std::exception& e) {
       Core::MessageManager::write(tr("Failed opening project '%1': %2").arg(QDir::toNativeSeparators(fileName)).arg(e.what()),
                                   Core::MessageManager::WithFocus);

       return NULL;
    }

    if (instantiated_targets.size() != 1) {
       Core::MessageManager::write(tr("Failed opening project '%1': Target instantiation produce more than one result").arg(QDir::toNativeSeparators(fileName)),
                                   Core::MessageManager::WithFocus);

       return NULL;
    }

    const main_target* topMainTarget = dynamic_cast<main_target*>(instantiated_targets[0]);
    HammerProject* mainProject = new HammerProject(this, topMainTarget, /*main_project=*/true);

    return mainProject;
}

ProjectExplorer::ProjectNode*
ProjectManager::add_dep(const main_target& mt, const HammerProject& owner)
{
   visible_targets_t::const_iterator vi = visible_targets_.find(&mt);
   if (vi != visible_targets_.end() && !vi->second)
      return NULL;

   stored_visible_targets_t::const_iterator svi = stored_visible_targets_.find(mt.location().string());
   if (svi != stored_visible_targets_.end())
      visible_targets_.insert(make_pair(&mt, svi->second));

   if (svi != stored_visible_targets_.end() && !svi->second)
      return NULL;

   deps_t::const_iterator i = deps_.find(&mt);
   if (i != deps_.end()) {
      HammerDepLinkProjectNode* result = new HammerDepLinkProjectNode(*i->second);
      return result;
   }

   HammerDepProjectNode* result = new HammerDepProjectNode(mt, owner);
   deps_.insert(make_pair(&mt, result));

   return result;
}

}}
