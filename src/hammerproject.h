#if !defined(h_51aa0b54_a286_48db_9fa4_f0b7328e74c2)
#define h_51aa0b54_a286_48db_9fa4_f0b7328e74c2

#include <projectexplorer/project.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildstep.h>
#include <coreplugin/idocument.h>
#include <QtCore/QFuture>

namespace ProjectExplorer{ class IProjectManager; }
namespace hammer{ class main_target; }

namespace hammer{ namespace QtCreator{

class ProjectManager;
class HammerProjectFile;
class HammerProjectNode;
class HammerBuildConfiguration;

class HammerProject : public ProjectExplorer::Project
{
      Q_OBJECT

   public:
      HammerProject(ProjectManager *manager, const main_target* mt, bool main_project = false);
      ~HammerProject();

      QString displayName() const override;
      Core::IDocument *document() const override;
      ProjectExplorer::IProjectManager* projectManager() const override;
      ProjectExplorer::ProjectNode *rootProjectNode() const override;
      QStringList files(FilesMode fileMode) const override;

      const main_target& get_main_target() const { return *m_mainTarget; }
      bool is_main_project() const { return main_project_; }
      void refresh();
   
   signals:
      void toolChainChanged(ProjectExplorer::ToolChain *);

   protected:
      RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;
   
   private:
      ProjectManager *m_manager;
      HammerProjectFile *m_projectFile;
      QString m_projectName;
      HammerProjectNode *m_rootNode;
      const main_target* m_mainTarget;
      QFuture<void> m_codeModelFuture;
      mutable QStringList m_files;
      bool main_project_;

      QStringList allIncludePaths(const hammer::main_target &mt) const;
      QStringList allDefines(const hammer::main_target& mt) const;
      QStringList files_impl(const hammer::main_target& mt,
                             FilesMode) const;
};

class HammerProjectFile : public Core::IDocument
{
    Q_OBJECT

   public:
      HammerProjectFile(HammerProject *parent, QString fileName);
      ~HammerProjectFile();

      bool save(QString *errorString, const QString &fileName, bool autoSave) override;

      QString defaultPath() const override;
      QString suggestedFileName() const override;

      bool isModified() const override;
      bool isFileReadOnly() const override;
      bool isSaveAsAllowed() const override;

      ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
      bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

   private:
      HammerProject *m_project;
      QString m_fileName;
};

}}

#endif //h_51aa0b54_a286_48db_9fa4_f0b7328e74c2
