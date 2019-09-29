#pragma once
#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <hammer/core/engine.h>

class QAction;

namespace hammer {

class engine;

}

namespace hammer { namespace QtCreator {

class HammerProject;

class ProjectManager : public ProjectExplorer::IProjectManager {
      Q_OBJECT

   public:
      struct cloned_state;

      ProjectManager();
      ~ProjectManager();

      QString mimeType() const override;

      ProjectExplorer::Project*
      openProject(const QString& fileName,
                  QString* errorString) override;

   private:
      QList<HammerProject*> m_projects;
      /// never null;
      std::unique_ptr<engine> engine_;

      QAction* reload_action_;

      void on_reload();
      void reload(cloned_state state);
};

}}
