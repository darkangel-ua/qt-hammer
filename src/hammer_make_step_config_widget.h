#pragma once
#include <QWidget>
#include <projectexplorer/buildstep.h>

namespace Ui {

class hammer_make_step_config_widget;

}

namespace hammer { namespace QtCreator {

class HammerMakeStep;

class hammer_make_step_config_widget : public ProjectExplorer::BuildStepConfigWidget {
      Q_OBJECT

   public:
      explicit hammer_make_step_config_widget(HammerMakeStep* make_step);
      ~hammer_make_step_config_widget();

      QString displayName() const override;
      QString summaryText() const override;

   private:
      Ui::hammer_make_step_config_widget* ui_;
      HammerMakeStep* make_step_;
      QString summary_text_;

   private slots:
      void updateDetails();
      void command_edit__TextEdited();
      void command_arguments__TextEdited();
};

}}
