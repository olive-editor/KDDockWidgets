/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2021 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "MyWidget.h"

#include <kddockwidgets/DockWidget.h>
#include <kddockwidgets/views/MainWindow_qtwidgets.h>
#include <kddockwidgets/views/DockWidget_qtwidgets.h>
#include <kddockwidgets/MDIArea.h>

#include "kddockwidgets/controllers/MainWindow.h"

#include <QStyleFactory>
#include <QApplication>
#include <QCommandLineParser>

// clazy:excludeall=qstring-allocations

int main(int argc, char **argv)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);

    app.setOrganizationName(QStringLiteral("KDAB"));
    app.setApplicationName(QStringLiteral("App supporting both docking and a MDI area"));

    KDDockWidgets::initFrontend(KDDockWidgets::FrontendType::QtWidgets);

    QCommandLineParser parser;
    parser.setApplicationDescription("KDDockWidgets MDI mixed with normal docking");
    parser.addHelpOption();

    QCommandLineOption nestedDocking("n", QCoreApplication::translate("main", "The MDI dock widgets will serve as drop areas, allowing for further nesting"));
    parser.addOption(nestedDocking);

    parser.process(app);

    // Fusion looks better in general, but feel free to change
    qApp->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // # 1. Create our main window

    KDDockWidgets::Views::MainWindow_qtwidgets mainWindow(QStringLiteral("MyMainWindow"), KDDockWidgets::MainWindowOption_HasCentralWidget);
    mainWindow.setWindowTitle("Main Window");
    mainWindow.resize(1600, 1200);
    mainWindow.show();

    // # 2. Create a dock widget, it needs a unique name
    auto dock1 = new KDDockWidgets::Views::DockWidget_qtwidgets(QStringLiteral("MyDock1"));
    auto widget1 = new MyWidget1();
    dock1->setWidget(widget1);

    auto dock2 = new KDDockWidgets::Views::DockWidget_qtwidgets(QStringLiteral("MyDock2"));
    auto widget2 = new MyWidget2();
    dock2->setWidget(widget2);

    // # 3. Dock them
    mainWindow.mainWindow()->addDockWidget(dock1->dockWidget(), KDDockWidgets::Location_OnLeft,
                                           nullptr, KDDockWidgets::InitialOption(QSize(300, 0)));
    mainWindow.mainWindow()->addDockWidget(dock2->dockWidget(), KDDockWidgets::Location_OnBottom,
                                           nullptr, KDDockWidgets::InitialOption(QSize(0, 300)));

    KDDockWidgets::Controllers::DockWidget::Options options = {};
    if (parser.isSet(nestedDocking)) {
        options |= KDDockWidgets::Controllers::DockWidget::Option_MDINestable;
    }

    // 4. Create our MDI widgets, which will go into the MDI area
    auto mdiWidget1 = new KDDockWidgets::Views::DockWidget_qtwidgets(QStringLiteral("MDI widget1"), options);
    mdiWidget1->setWidget(new MyWidget1());

    auto mdiWidget2 = new KDDockWidgets::Views::DockWidget_qtwidgets(QStringLiteral("MDI widget2"), options);
    mdiWidget2->setWidget(new MyWidget2());

    auto mdiWidget3 = new KDDockWidgets::Views::DockWidget_qtwidgets(QStringLiteral("MDI widget3"), options);
    mdiWidget3->setWidget(new MyWidget3());

    auto mdiArea = new KDDockWidgets::MDIArea();
    // TODOm2: Add View::MainWindow::setPersistentCentralWidget
    auto mdiAreaW = std::shared_ptr<KDDockWidgets::ViewWrapper>(new KDDockWidgets::Views::ViewWrapper_qtwidgets(mdiArea));
    mainWindow.mainWindow()->setPersistentCentralWidget(mdiAreaW);

    mdiArea->addDockWidget(mdiWidget1->dockWidget(), QPoint(10, 10));
    mdiArea->addDockWidget(mdiWidget2->dockWidget(), QPoint(50, 50));
    mdiArea->addDockWidget(mdiWidget3->dockWidget(), QPoint(110, 110));


    return app.exec();
}
