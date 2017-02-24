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

      FolderNode* m_srcNode = nullptr;
      FolderNode* m_incNode = nullptr;
      FolderNode* m_resNode = nullptr;
      FolderNode* m_formNode = nullptr;
      FolderNode* m_buildNode = nullptr;
};

class HammerProjectNode : public HammerNodeBase
{
   public:
      HammerProjectNode(HammerProject* project, Core::IDocument* projectFile);
      ~HammerProjectNode();
      bool canAddSubProject(const QString &proFilePath) const override;
      bool addSubProjects(const QStringList &proFilePaths) override;
      bool removeSubProjects(const QStringList &proFilePaths) override;

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

      bool hasBuildTargets() const { return false; }

      bool canAddSubProject(const QString &proFilePath) const override { return false; }

      bool addSubProjects(const QStringList &proFilePaths) override { return false; }
      bool removeSubProjects(const QStringList &proFilePaths) override { return false; }

      bool addFiles(const QStringList &filePaths,
                    QStringList *notAdded = 0) override
      { return false; }

      bool removeFiles(const QStringList &filePaths,
                       QStringList *notRemoved = 0) override
      { return false; }

      bool deleteFiles(const QStringList &filePaths) override
      { return false; }

      bool renameFile(const QString &filePath,
                      const QString &newFilePath) override
      { return false; }

      QList<ProjectExplorer::RunConfiguration*>
      runConfigurations() const override
      { return QList<ProjectExplorer::RunConfiguration*>(); }

      void refresh() override;
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

      bool canAddSubProject(const QString &proFilePath) const override { return false; }

      bool addSubProjects(const QStringList &proFilePaths) override { return false; }
      bool removeSubProjects(const QStringList &proFilePaths) override { return false; }

      bool addFiles(const QStringList &filePaths,
                    QStringList *notAdded = 0) override
      { return false; }

      bool removeFiles(const QStringList &filePaths,
                       QStringList *notRemoved = 0) override
      { return false; }

      bool deleteFiles(const QStringList &filePaths) override
      { return false; }

      bool renameFile(const QString &filePath,
                      const QString &newFilePath) override
      { return false; }

      QList<ProjectExplorer::RunConfiguration *>
      runConfigurations() const override
      { return QList<ProjectExplorer::RunConfiguration *>(); }

      void refresh();
      HammerDepProjectNode& link() { return link_; }

   private:
      HammerDepProjectNode& link_;
};

}}

#endif //h_e94fdb51_b369_4d9f_b686_5a6db23a9466
