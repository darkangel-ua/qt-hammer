#pragma once
#include <projectexplorer/localapplicationrunconfiguration.h>
#include <boost/optional.hpp>

namespace hammer { namespace QtCreator {

class HammerRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration {
      Q_OBJECT
      friend class HammerRunConfigurationFactory;
      friend class HammerRunConfigurationWidget;
   public:
      explicit HammerRunConfiguration(ProjectExplorer::Target* parent);
      QString executable() const override;

      ProjectExplorer::ApplicationLauncher::Mode
      runMode() const override;

      QString workingDirectory() const override;
      QString commandLineArguments() const override;
      QWidget* createConfigurationWidget() override;
      bool isConfigured() const override { return true; }
      ConfigurationState ensureConfigured(QString* errorMessage = 0) override { return Configured; }

   signals:
       void baseWorkingDirectoryChanged(const QString&);

   protected:
      HammerRunConfiguration(ProjectExplorer::Target* parent,
                             HammerRunConfiguration* source);
      bool fromMap(const QVariantMap &map) override;
      QVariantMap toMap() const override;

   private:
      ProjectExplorer::Target* m_target;
      mutable boost::optional<QString> m_executable;
      mutable boost::optional<QStringList> m_additionalPaths;

      QString m_workingDirectory;
      void setBaseWorkingDirectory(const QString& workingDirectory);
      QString baseWorkingDirectory() const;
};

class HammerRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory {
      Q_OBJECT
   public:
      explicit HammerRunConfigurationFactory(QObject* parent = NULL);

      QList<Core::Id>
      availableCreationIds(ProjectExplorer::Target* parent,
                           CreationMode mode = UserCreate) const override;

      QString displayNameForId(Core::Id id) const override;

      bool canCreate(ProjectExplorer::Target* parent,
                     Core::Id id) const override;

      bool canRestore(ProjectExplorer::Target* parent,
                      const QVariantMap &map) const override;

      bool canClone(ProjectExplorer::Target* parent,
                    ProjectExplorer::RunConfiguration*source) const override;

      ProjectExplorer::RunConfiguration*
      clone(ProjectExplorer::Target* parent,
            ProjectExplorer::RunConfiguration* source) override;

   private:
      ProjectExplorer::RunConfiguration*
      doCreate(ProjectExplorer::Target* parent,
               Core::Id id) override;

      ProjectExplorer::RunConfiguration*
      doRestore(ProjectExplorer::Target* parent,
                const QVariantMap& map) override;
};

}}
