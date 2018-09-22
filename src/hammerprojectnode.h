#pragma once
#include <projectexplorer/projectnodes.h>

namespace Core {

class IDocument;

}

namespace hammer {

class basic_target;
class main_target;

}

namespace hammer { namespace QtCreator {

class HammerProject;

class HammerProjectNode : public ProjectExplorer::ProjectNode {
   public:
      HammerProjectNode(HammerProject* project,
                        Core::IDocument* projectFile);
      ~HammerProjectNode();

      bool canAddSubProject(const QString &proFilePath) const override;
      bool addSubProjects(const QStringList &proFilePaths) override;
      bool removeSubProjects(const QStringList &proFilePaths) override;
      void refresh();

   private:
      typedef QHash<QString, FolderNode*> FolderByName;

      HammerProject* m_project;
      Core::IDocument* m_projectFile;

      FolderNode* m_srcNode = nullptr;
      FolderNode* m_incNode = nullptr;
      FolderNode* m_resNode = nullptr;
      FolderNode* m_formNode = nullptr;
      FolderNode* m_buildNode = nullptr;

      void addNodes(const basic_target* bt);
};

}}
