#if !defined(h_a1cc270b_d0ff_4867_ad78_41400c859475)
#define h_a1cc270b_d0ff_4867_ad78_41400c859475

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <hammer/core/engine.h>

class QAction;

namespace hammer {

class engine;

}

namespace hammer{ namespace QtCreator{

class HammerProject;

class ProjectManager : public ProjectExplorer::IProjectManager
{
      Q_OBJECT

   public:
      ProjectManager();
      ~ProjectManager();

      QString mimeType() const override;
      ProjectExplorer::Project *openProject(const QString& fileName,
                                            QString *errorString) override;
   private:
      QList<HammerProject*> m_projects;
      /// never null;
      std::unique_ptr<engine> engine_;

      QAction* reload_action_;

      void on_reload();
};

}}

#endif
