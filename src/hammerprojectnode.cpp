#include <coreplugin/idocument.h>
#include <QFileInfo>

#include <boost/foreach.hpp>
#include <boost/unordered_set.hpp>
#include <hammer/core/main_target.h>
#include <hammer/core/types.h>
#include <hammer/core/target_type.h>
#include <hammer/core/feature_set.h>
#include <hammer/core/feature.h>
#include <hammer/core/toolsets/qt_toolset.h>

#include "hammerprojectnode.h"
#include "hammerproject.h"
#include "hammerprojectmanager.h"

namespace hammer { namespace QtCreator {

void gatherAllMainTargets(boost::unordered_set<const main_target*>& targets,
                          const main_target& targetToInspect);

static
std::string
versioned_name(const hammer::main_target& mt)
{
   const std::string& name = mt.name();
   const feature_set& props = mt.properties();
   auto i = props.find("version");
   if (i == props.end())
      return name;
   else
      return name + "-" + (**i).value();
}

HammerProjectNode::HammerProjectNode(HammerProject* project, 
                                     Core::IDocument* projectFile)
   : ProjectExplorer::ProjectNode(projectFile->filePath()),
     m_project(project),
     m_projectFile(projectFile)
{
   setDisplayName(QString::fromStdString(versioned_name(project->get_main_target())));
   refresh();
}

HammerProjectNode::~HammerProjectNode()
{

}

bool HammerProjectNode::canAddSubProject(const QString &proFilePath) const
{
   return false;
}

bool HammerProjectNode::addSubProjects(const QStringList &proFilePaths)
{
   return false;
}

bool HammerProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
   return false;
}

void HammerProjectNode::addNodes(const basic_target* bt)
{
   if (bt->type().equal_or_derived_from(types::CPP) ||
       bt->type().equal_or_derived_from(types::C))
   {
      ProjectExplorer::FileNode* f = new ProjectExplorer::FileNode(Utils::FileName::fromString(QString::fromStdString(bt->full_path().string())), ProjectExplorer::SourceType, false);
      if (!m_srcNode) {
         m_srcNode = new FolderNode(Utils::FileName::fromString(QString("src")));
         addFolderNodes({m_srcNode});
      }

      m_srcNode->addFileNodes(QList<ProjectExplorer::FileNode*>() << f);
   } else if (bt->type().equal_or_derived_from(types::H)) {
      ProjectExplorer::FileNode* f = new ProjectExplorer::FileNode(Utils::FileName::fromString(QString::fromStdString(bt->full_path().string())), ProjectExplorer::HeaderType, false);
      if (!m_incNode) {
         m_incNode = new FolderNode(Utils::FileName::fromString(QString("include")));
         addFolderNodes({m_incNode});
      }

      m_incNode->addFileNodes(QList<ProjectExplorer::FileNode*>() << f);
   } else if (bt->type().equal_or_derived_from(hammer::qt_ui)) {
      ProjectExplorer::FileNode* f = new ProjectExplorer::FileNode(Utils::FileName::fromString(QString::fromStdString(bt->full_path().string())), ProjectExplorer::FormType, false);
      if (!m_formNode) {
         m_formNode = new FolderNode(Utils::FileName::fromString(QString("forms")));
         addFolderNodes({m_formNode});
      }
      m_formNode->addFileNodes(QList<ProjectExplorer::FileNode*>() << f);
   } else if (bt->type().equal_or_derived_from(hammer::qt_rc)) {
      ProjectExplorer::FileNode* f = new ProjectExplorer::FileNode(Utils::FileName::fromString(QString::fromStdString(bt->full_path().string())), ProjectExplorer::ResourceType, false);
      if (!m_resNode) {
         m_resNode = new FolderNode(Utils::FileName::fromString(QString("resources")));
         addFolderNodes({m_resNode});
      }
      m_resNode->addFileNodes(QList<ProjectExplorer::FileNode*>() << f);
   } else if (bt->type().equal_or_derived_from(hammer::qt_uic_main) ||
              bt->type().equal_or_derived_from(hammer::types::PCH) ||
              bt->type().equal_or_derived_from(hammer::types::OBJ))
   {
      BOOST_FOREACH(const basic_target* i, static_cast<const main_target*>(bt)->sources())
        addNodes(i);
   }
}

void HammerProjectNode::refresh()
{
   removeFileNodes(fileNodes());
   removeProjectNodes(subProjectNodes());
   removeFolderNodes(subFolderNodes());
   m_srcNode = NULL;
   m_incNode = NULL;
   m_resNode = NULL;
   m_formNode = NULL;

   m_buildNode = new FolderNode(Utils::FileName::fromString(QString("build")));
   addFolderNodes({m_buildNode});

   ProjectExplorer::FileNode* hamfile_node = new ProjectExplorer::FileNode(m_projectFile->filePath(), ProjectExplorer::ProjectFileType, false);
   m_buildNode->addFileNodes({hamfile_node});

   BOOST_FOREACH(const basic_target* bt, m_project->get_main_target().sources())
      addNodes(bt);
}

}}
