#if !defined(h_7a123cb7_7fc3_4180_ae0c_6c4d3c626198)
#define h_7a123cb7_7fc3_4180_ae0c_6c4d3c626198

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/namedwidget.h>

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
class ToolChain;
}

namespace hammer{ namespace QtCreator{

class HammerBuildConfigurationFactory;

class HammerBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
      Q_OBJECT
      friend class HammerBuildConfigurationFactory;
   
   public:
      HammerBuildConfiguration(ProjectExplorer::Target* parent);
      ~HammerBuildConfiguration();
      ProjectExplorer::NamedWidget *createConfigWidget() override;

//      HammerTarget *hammerTarget() const;

//      QVariantMap toMap() const override;

      BuildType buildType() const;
//      virtual bool fromMap(const QVariantMap &map);

   protected:
      HammerBuildConfiguration(ProjectExplorer::Target* parent,
                               HammerBuildConfiguration* source);
};

class HammerBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
      Q_OBJECT

   public:
      HammerBuildConfigurationFactory(QObject* parent = 0);
      ~HammerBuildConfigurationFactory();

      int priority(const ProjectExplorer::Target *parent) const override;
      QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
      int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
      QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k,
                                                          const QString &projectPath) const override;

      HammerBuildConfiguration* create(ProjectExplorer::Target* parent,
                                       const ProjectExplorer::BuildInfo* info) const override;
      bool canClone(const ProjectExplorer::Target* parent,
                    ProjectExplorer::BuildConfiguration* source) const override;
      HammerBuildConfiguration *clone(ProjectExplorer::Target *parent,
                                      ProjectExplorer::BuildConfiguration *source) override;
      bool canRestore(const ProjectExplorer::Target* parent,
                      const QVariantMap& map) const override;
      HammerBuildConfiguration *restore(ProjectExplorer::Target *parent,
                                        const QVariantMap &map) override;
   private:
      bool canHandle(const ProjectExplorer::Target *t) const;
      ProjectExplorer::BuildInfo* createBuildInfo(const ProjectExplorer::Kit* k,
                                                  const Utils::FileName& buildDir) const;
};

class HammerBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    HammerBuildSettingsWidget(HammerBuildConfiguration* bc);

private slots:
    void buildDirectoryChanged();
    void environmentHasChanged();

private:
    Utils::PathChooser *m_pathChooser;
    HammerBuildConfiguration *m_buildConfiguration;
};

}} 

#endif //h_7a123cb7_7fc3_4180_ae0c_6c4d3c626198
