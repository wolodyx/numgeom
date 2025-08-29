#include <QGuiApplication>
#include <QVulkanInstance>
#include <QLoggingCategory>

#include "vulkanwindow.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;
    //inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
    if(!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow w;
    w.setVulkanInstance(&inst);

    w.resize(1024, 768);
    w.show();

    return app.exec();
}

