#if !defined(h_2d3dca1c_c0f1_4607_b14f_ba5806df07cc)
#define h_2d3dca1c_c0f1_4607_b14f_ba5806df07cc

#include <projectexplorer/abstractprocessstep.h>

class QListWidgetItem;

namespace hammer{ namespace QtCreator{

class HammerBuildConfiguration;
class hammer_make_step_config_widget;
class HammerMakeStepFactory;

class HammerMakeStep : public ProjectExplorer::AbstractProcessStep
{
      Q_OBJECT
      friend class hammer_make_step_config_widget;
      friend class HammerMakeStepFactory;

   public:
      HammerMakeStep(ProjectExplorer::BuildStepList *parent);
      virtual ~HammerMakeStep();

      HammerBuildConfiguration *hammerBuildConfiguration() const;

      bool init() override;

      void run(QFutureInterface<bool> &fi) override;

      ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
      bool immutable() const override;
      void set_arguments(const QString& args);
      QString allArguments() const;
      QString makeCommand() const;

      QVariantMap toMap() const;

   protected:
      HammerMakeStep(ProjectExplorer::BuildStepList *parent, HammerMakeStep *bs);
      bool fromMap(const QVariantMap &map) override;

   private:
      void ctor();

      QString m_makeArguments;
      QString m_makeCommand;
};

class HammerMakeCurrentStep : public ProjectExplorer::AbstractProcessStep
{
      Q_OBJECT
//      friend class HammerMakeStepConfigWidget; // TODO remove again?
      friend class HammerMakeStepFactory;

   public:
      HammerMakeCurrentStep(ProjectExplorer::BuildStepList *parent);

      HammerBuildConfiguration *hammerBuildConfiguration() const;
      virtual bool init();
      virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
      virtual bool immutable() const { return false; }
      void setTargetToBuid(const QString& target, const QString& projectPath);

   protected:
      HammerMakeCurrentStep(ProjectExplorer::BuildStepList *parent, HammerMakeCurrentStep *bs);
   
   private:
      QString m_targetToBuild;
};

class HammerMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
      Q_OBJECT
   public:
      explicit HammerMakeStepFactory(QObject *parent = 0);
      ~HammerMakeStepFactory();

      bool canCreate(ProjectExplorer::BuildStepList* parent,
                     Core::Id id) const override;
      ProjectExplorer::BuildStep* create(ProjectExplorer::BuildStepList *parent,
                                         const Core::Id id) override;
      bool canClone(ProjectExplorer::BuildStepList *parent,
                    ProjectExplorer::BuildStep *source) const override;
      ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                        ProjectExplorer::BuildStep *source) override;
      bool canRestore(ProjectExplorer::BuildStepList *parent,
                      const QVariantMap &map) const override;
      ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
                                          const QVariantMap &map) override;

      QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const override;
      QString displayNameForId(Core::Id id) const override;
};

}}

#endif //h_2d3dca1c_c0f1_4607_b14f_ba5806df07cc
