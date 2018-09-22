#pragma once
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/namedwidget.h>

namespace Utils {

class PathChooser;

}

namespace ProjectExplorer {

class ToolChain;

}

namespace hammer { namespace QtCreator {

class HammerBuildConfigurationFactory;

class HammerBuildConfiguration : public ProjectExplorer::BuildConfiguration {
      Q_OBJECT
      friend class HammerBuildConfigurationFactory;
   
   public:
      HammerBuildConfiguration(ProjectExplorer::Target* parent);
      ~HammerBuildConfiguration();
      ProjectExplorer::NamedWidget *createConfigWidget() override;

      BuildType buildType() const override;

   protected:
      HammerBuildConfiguration(ProjectExplorer::Target* parent,
                               HammerBuildConfiguration* source);

      BuildType build_type_ = BuildType::Debug;
};

class HammerBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory {
      Q_OBJECT

   public:
      HammerBuildConfigurationFactory(QObject* parent = 0);
      ~HammerBuildConfigurationFactory();

      int priority(const ProjectExplorer::Target* parent) const override;

      QList<ProjectExplorer::BuildInfo*>
      availableBuilds(const ProjectExplorer::Target* parent) const override;

      int priority(const ProjectExplorer::Kit* k,
                   const QString &projectPath) const override;

      QList<ProjectExplorer::BuildInfo*>
      availableSetups(const ProjectExplorer::Kit* k,
                      const QString &projectPath) const override;

      HammerBuildConfiguration*
      create(ProjectExplorer::Target* parent,
             const ProjectExplorer::BuildInfo* info) const override;

      bool canClone(const ProjectExplorer::Target* parent,
                    ProjectExplorer::BuildConfiguration* source) const override;

      HammerBuildConfiguration*
      clone(ProjectExplorer::Target* parent,
            ProjectExplorer::BuildConfiguration* source) override;

      bool canRestore(const ProjectExplorer::Target* parent,
                      const QVariantMap& map) const override;

      HammerBuildConfiguration*
      restore(ProjectExplorer::Target* parent,
              const QVariantMap &map) override;

   private:
      bool canHandle(const ProjectExplorer::Target* t) const;

      QList<ProjectExplorer::BuildInfo*>
      createBuildInfo(const ProjectExplorer::Kit* k,
                      const Utils::FileName& buildDir) const;
};

}} 
