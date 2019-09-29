#include "hammerplugin.h"
#include "hammerprojectmanager.h"
#include "hammerbuildconfiguration.h"
#include "hammerrunconfiguration.h"
#include "hammermakestep.h"
#include <utils/mimetypes/mimedatabase.h>

HammerPlugin::HammerPlugin()
{

}

HammerPlugin::~HammerPlugin()
{

}

bool HammerPlugin::initialize(const QStringList &arguments,
                              QString *errorMessage)
{
   Q_UNUSED(arguments)

   Utils::MimeDatabase::addMimeTypes(QLatin1String(":hammerproject/HammerProject.mimetypes.xml"));

   hammer::QtCreator::ProjectManager *manager = new hammer::QtCreator::ProjectManager();
   addAutoReleasedObject(manager);
   addAutoReleasedObject(new hammer::QtCreator::HammerMakeStepFactory);
   addAutoReleasedObject(new hammer::QtCreator::HammerBuildConfigurationFactory);
   addAutoReleasedObject(new hammer::QtCreator::HammerRunConfigurationFactory);

   return true;

}

void HammerPlugin::extensionsInitialized()
{

}
