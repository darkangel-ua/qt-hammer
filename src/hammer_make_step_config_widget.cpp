#include "hammer_make_step_config_widget.h"
#include "ui_hammer_make_step_config_widget.h"
#include "hammerbuildconfiguration.h"
#include "hammermakestep.h"

namespace hammer{ namespace QtCreator{

hammer_make_step_config_widget::hammer_make_step_config_widget(HammerMakeStep* make_step) :
   ui_(new Ui::hammer_make_step_config_widget),
   make_step_(make_step)
{
   ui_->setupUi(this);
   ui_->command_edit_->setText(make_step_->makeCommand());
   ui_->command_arguments_edit_->setText(make_step_->allArguments());

   connect(ui_->command_edit_, SIGNAL(textEdited(QString)),
           this, SLOT(command_edit__TextEdited()));

   connect(ui_->command_arguments_edit_, SIGNAL(textEdited(QString)),
           this, SLOT(command_arguments__TextEdited()));

   updateDetails();
}

hammer_make_step_config_widget::~hammer_make_step_config_widget()
{
   delete ui_;
}

void hammer_make_step_config_widget::command_edit__TextEdited()
{
   make_step_->m_makeCommand = ui_->command_edit_->text();
   updateDetails();
}

void hammer_make_step_config_widget::command_arguments__TextEdited()
{
   make_step_->m_makeArguments = ui_->command_arguments_edit_->text();
   updateDetails();
}

QString hammer_make_step_config_widget::displayName() const
{
   return "Hammer";
}

QString hammer_make_step_config_widget::summaryText() const
{
   return summary_text_;
}

void hammer_make_step_config_widget::updateDetails()
{
    HammerBuildConfiguration* bc = make_step_->hammerBuildConfiguration();

    ProjectExplorer::ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(make_step_->makeCommand());
    param.setArguments(make_step_->allArguments());
    summary_text_ = param.summary(displayName());
    emit updateSummary();
}

}}
