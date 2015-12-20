#if !defined(h_e94fdb51_b369_4d9f_b686_5a6db23a9466)
#define h_e94fdb51_b369_4d9f_b686_5a6db23a9466

#include <projectexplorer/projectnodes.h>

namespace Core
{
   class IDocument;
}

namespace hammer {

class basic_target;
class main_target;

}

namespace hammer{namespace QtCreator{


class HammerProject;
class HammerDepProjectNode;

class HammerNodeBase : public ProjectExplorer::ProjectNode
{
   protected:
      HammerNodeBase(const Utils::FileName& projectFilePath);
      void addNodes(const basic_target* bt);
      virtual void refresh();

      FolderNode* m_srcNode;
      FolderNode* m_incNode;
      FolderNode* m_resNode;
      FolderNode* m_formNode;
};

class HammerProjectNode : public HammerNodeBase
{
   public:
      HammerProjectNode(HammerProject* project, Core::IDocument* projectFile);
      ~HammerProjectNode();
      bool canAddSubProject(const QString &proFilePath) const override;
      bool addSubProjects(const QStringList &proFilePaths) override;
      bool removeSubProjects(const QStringList &proFilePaths) override;

//      Core::IDocument* projectFile() const;
//      QString projectFilePath() const;

   private:
      typedef QHash<QString, FolderNode *> FolderByName;

      HammerProject* m_project;
      Core::IDocument* m_projectFile;

      void refresh();
};

class HammerDepProjectNode : public HammerNodeBase
{
   public:
      HammerDepProjectNode(const hammer::main_target& mt,
                           const HammerProject& owner);
      ~HammerDepProjectNode();

      virtual bool hasBuildTargets() const { return false; }

      virtual bool canAddSubProject(const QString &proFilePath) const { return false; }

      virtual bool addSubProjects(const QStringList &proFilePaths)  { return false; }
      virtual bool removeSubProjects(const QStringList &proFilePaths)  { return false; }

      virtual bool addFiles(const ProjectExplorer::FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0) { return false; }

      virtual bool removeFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0) { return false; }
      virtual bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths) { return false; }

      virtual bool renameFile(const ProjectExplorer::FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath) { return false; }

      virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node) { return QList<ProjectExplorer::RunConfiguration *>(); }
      void refresh();
      const HammerProject& owner() const { return owner_; }
      const hammer::main_target& mt() const { return mt_; }

   private:
      const hammer::main_target& mt_;
      const HammerProject& owner_;
};

class HammerDepLinkProjectNode : public HammerNodeBase
{
   public:
      HammerDepLinkProjectNode(HammerDepProjectNode& link);
      ~HammerDepLinkProjectNode();

      virtual bool hasBuildTargets() const { return false; }
      virtual bool canAddSubProject(const QString &proFilePath) const { return false; }

      virtual bool addSubProjects(const QStringList &proFilePaths)  { return false; }
      virtual bool removeSubProjects(const QStringList &proFilePaths)  { return false; }

      virtual bool addFiles(const ProjectExplorer::FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0) { return false; }

      virtual bool removeFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0) { return false; }
      virtual bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths) { return false; }

      virtual bool renameFile(const ProjectExplorer::FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath) { return false; }

      virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node) { return QList<ProjectExplorer::RunConfiguration *>(); }
      void refresh();
      HammerDepProjectNode& link() { return link_; }

   private:
      HammerDepProjectNode& link_;
};

}}

#endif //h_e94fdb51_b369_4d9f_b686_5a6db23a9466
