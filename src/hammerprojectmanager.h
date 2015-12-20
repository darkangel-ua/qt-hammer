#if !defined(h_a1cc270b_d0ff_4867_ad78_41400c859475)
#define h_a1cc270b_d0ff_4867_ad78_41400c859475

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <hammer/core/engine.h>

namespace hammer{namespace QtCreator{

class HammerProject;
class HammerDepProjectNode;

typedef std::map<const ::hammer::main_target*, bool> visible_targets_t;
typedef std::map<std::string /*tag*/, bool> stored_visible_targets_t;

class ProjectManager : public ProjectExplorer::IProjectManager
{
      Q_OBJECT

   public:
      ProjectManager();
      ~ProjectManager();

      QString mimeType() const override;
      ProjectExplorer::Project *openProject(const QString& fileName,
                                            QString *errorString) override;
      ProjectExplorer::ProjectNode* add_dep(const main_target& mt,
                                            const HammerProject& owner);
   private:
      typedef std::map<const main_target*, HammerDepProjectNode*> deps_t;

      QList<HammerProject*> m_projects;
      engine m_engine;
      deps_t deps_;
      visible_targets_t visible_targets_;
      stored_visible_targets_t stored_visible_targets_;
};

}}

#endif //h_a1cc270b_d0ff_4867_ad78_41400c859475
