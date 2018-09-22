#pragma once
#include <extensionsystem/iplugin.h>

class HammerPlugin : public ExtensionSystem::IPlugin {
      Q_OBJECT
      Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Hammer.json")

   public:
      HammerPlugin();
      ~HammerPlugin();

      bool initialize(const QStringList &arguments,
                      QString *errorMessage) override;
      void extensionsInitialized() override;
};
