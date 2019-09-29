#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchainmanager.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/nodesvisitor.h>
#include <extensionsystem/pluginmanager.h>
#include <cpptools/cppmodelmanager.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <hammer/core/main_target.h>
#include <hammer/core/feature_set.h>
#include <hammer/core/feature.h>
#include <hammer/core/types.h>
#include <hammer/core/target_type.h>
#include <hammer/core/testing_run_meta_target.h>
#include <hammer/core/testing_intermediate_meta_target.h>

#include "hammerprojectmanager.h"
#include "hammerproject.h"
#include "hammerprojectconstants.h"
#include "hammerprojectnode.h"
#include "hammerrunconfiguration.h"

using ProjectExplorer::Kit;
using ProjectExplorer::KitManager;
using ProjectExplorer::Target;

namespace hammer{ namespace QtCreator{

static
const main_target&
skip_testing_run_if_any(const main_target& mt)
{
   const testing_run_meta_target* tmt = dynamic_cast<const testing_run_meta_target*>(mt.get_meta_target());

   if (!tmt)
      return mt;

   for (const basic_target* bt : mt.sources()) {
      if (bt->type().equal_or_derived_from(types::EXE) &&
          dynamic_cast<const testing_intermediate_meta_target*>(bt->get_meta_target()))
      {
         return *static_cast<const main_target*>(bt);
      }
   }

   return mt;
}

HammerProject::HammerProject(ProjectManager* manager,
                             const main_target* mt,
                             bool main_project)
   : m_manager(manager),
     m_mainTarget(mt),
     main_project_(main_project)
{
   setId(HAMMERPROJECT_ID);
   setProjectContext(Core::Context(PROJECTCONTEXT));
   setProjectLanguages(Core::Context(ProjectExplorer::Constants::LANG_CXX));
   
   QFileInfo fileInfo(QString::fromStdString((m_mainTarget->location() / "hamfile").string()));

   m_projectName = QString::fromStdString(mt->name());
   m_projectFile = new HammerProjectFile(this, fileInfo.absoluteFilePath());
   m_rootNode = new HammerProjectNode(this, m_projectFile);
}

HammerProject::~HammerProject()
{
   m_codeModelFuture.cancel();

   delete m_rootNode;
}

QString
HammerProject::displayName() const
{
   return m_projectName;
}

Core::IDocument*
HammerProject::document() const
{
   return m_projectFile;
}

ProjectExplorer::IProjectManager*
HammerProject::projectManager() const
{
   return m_manager;
}

ProjectExplorer::ProjectNode*
HammerProject::rootProjectNode() const
{
   return m_rootNode;
}

QStringList
HammerProject::files(FilesMode fileMode) const
{
   if (!m_files.isEmpty())
      return m_files;

   m_files += files_impl(*m_mainTarget, fileMode);
   m_files += m_projectFile->filePath().toString();

   return m_files;
}

QStringList
HammerProject::files_impl(const hammer::main_target& mt,
                          FilesMode) const
{
   QStringList result;

   for(const basic_target* bt : skip_testing_run_if_any(mt).sources()) {
      if (bt->type().equal_or_derived_from(types::CPP) ||
          bt->type().equal_or_derived_from(types::C))
      {
         result.append(QString::fromStdString(bt->full_path().string()));
      }
   }

   return result;
}

void HammerProject::refresh()
{
   CppTools::CppModelManager* modelManager = CppTools::CppModelManager::instance();

   if (modelManager) {
      CppTools::ProjectInfo pinfo = modelManager->projectInfo(this);
      //FIXME: Begin. I don't know WHY it doesn't work without these two lines
      if (!pinfo.isValid())
         pinfo = CppTools::ProjectInfo(this);
      //FIXME: End

      CppTools::ProjectPartBuilder ppBuilder(pinfo);
      ppBuilder.setIncludePaths(allIncludePaths(*m_mainTarget));
      ppBuilder.setDefines(allDefines(*m_mainTarget).join("\n").toLocal8Bit());
      const QList<Core::Id> languages = ppBuilder.createProjectPartsForFiles(files_impl(*m_mainTarget, AllFiles));
      for (Core::Id language : languages)
         setProjectLanguage(language, true);

      m_codeModelFuture.cancel();
      pinfo.finish();
      modelManager->updateProjectInfo(pinfo);
   }

   m_files.clear();
   emit fileListChanged();
}

void HammerProject::reload(const main_target* mt)
{
   Q_ASSERT(mt);
   m_mainTarget = mt;

   refresh();
   m_rootNode->refresh();
}

QStringList
HammerProject::allIncludePaths(const hammer::main_target& mt) const
{
   QStringList result;

   for (feature_ref f : skip_testing_run_if_any(mt).properties()) {
      if (f->name() == "include") {
         location_t l = f->get_path_data().project_->location() / f->value();
         l.normalize();
         result.append(QString::fromStdString(l.string()));
      }
   }

   return result;
}

QStringList
HammerProject::allDefines(const hammer::main_target& mt) const
{
   QStringList result;
   for (feature_ref f : skip_testing_run_if_any(mt).properties()) {
      if (f->name() == "define") {
         QString v(f->value().c_str());
         v.replace(QString("="), QString(" "));
         result.append("#define " + v);
      }
   }

   return result;
}

ProjectExplorer::Project::RestoreResult
HammerProject::fromMap(const QVariantMap& map,
                       QString* errorMessage)
{
   RestoreResult r = Project::fromMap(map, errorMessage);
   if (r != RestoreResult::Ok)
      return r;

   Kit* defaultKit = KitManager::defaultKit();
   if (!activeTarget() && defaultKit)
      addTarget(createTarget(defaultKit));

   QList<Target*> targetList = targets();
   for (Target* t : targetList) {
      if (!t->buildConfigurations().empty())
         continue;
   }

   refresh();

   return RestoreResult::Ok;
}

HammerProjectFile::HammerProjectFile(HammerProject* parent,
                                     QString fileName)
   : Core::IDocument(parent),
     m_project(parent),
     m_fileName(fileName)
{
   setId("Hammer.ProjectFile");
   setMimeType(QLatin1String(HAMMERMIMETYPE));
   setFilePath(Utils::FileName::fromString(fileName));
}

HammerProjectFile::~HammerProjectFile()
{
}

bool HammerProjectFile::save(QString*,
                             const QString&,
                             bool)
{
    return false;
}

QString HammerProjectFile::defaultPath() const
{
    return QString();
}

QString HammerProjectFile::suggestedFileName() const
{
    return QString();
}

bool HammerProjectFile::isModified() const
{
    return false;
}

bool HammerProjectFile::isFileReadOnly() const
{
    return true;
}

bool HammerProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior
HammerProjectFile::reloadBehavior(ChangeTrigger,
                                  ChangeType) const
{
    return BehaviorSilent;
}

bool HammerProjectFile::reload(QString*,
                               ReloadFlag,
                               ChangeType)
{
    return true;
}

}}
