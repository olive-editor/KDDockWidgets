/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

// We don't care about performance related checks in the tests
// clazy:excludeall=ctor-missing-parent-argument,missing-qobject-macro,range-loop,missing-typeinfo,detaching-member,function-args-by-ref,non-pod-global-static,reserve-candidates,qstring-allocations

#include "Testing.h"
#include "utils.h"
#include "DockWidgetBase.h"
#include "multisplitter/Separator_p.h"
#include "private/MultiSplitter_p.h"
#include "TitleBar_p.h"
#include "Position_p.h"
#include "DropAreaWithCentralFrame_p.h"
#include "WindowBeingDragged_p.h"

#include <QtTest/QtTest>
#include <QObject>
#include <QApplication>

#ifdef KDDOCKWIDGETS_QTQUICK
# include "quick/DockWidgetQuick.h"

# include <QQmlEngine>
# include <QQuickStyle>
# else
# include "DockWidget.h"
# include <QPushButton>
#endif

using namespace KDDockWidgets;
using namespace Layouting;
using namespace KDDockWidgets::Tests;

class TestCommon : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase()
    {
        qputenv("KDDOCKWIDGETS_SHOW_DEBUG_WINDOW", "");
        qApp->setOrganizationName("KDAB");
        qApp->setApplicationName("dockwidgets-unit-tests");

        Testing::installFatalMessageHandler();

#ifdef KDDOCKWIDGETS_QTQUICK
        QQuickStyle::setStyle("Material"); // so we don't load KDE pluginss
        KDDockWidgets::Config::self().setQmlEngine(new QQmlEngine(this));
#endif
        QTest::qWait(10); // the DND state machine needs the event loop to start, otherwise activeState() is nullptr. (for offscreen QPA)
    }

    void cleanupTestCase()
    {
#ifdef KDDOCKWIDGETS_QTQUICK
        delete KDDockWidgets::Config::Config::self().qmlEngine();
#endif
    }

private Q_SLOTS:
    void tst_simple1();
    void tst_simple2();
    void tst_doesntHaveNativeTitleBar();
    void tst_resizeWindow2();
    void tst_hasLastDockedLocation();
    void tst_ghostSeparator();
    void tst_detachFromMainWindow();
    void tst_detachPos();
    void tst_floatingWindowSize();
    void tst_sizeAfterRedock();
    void tst_tabbingWithAffinities();
    void tst_honourUserGeometry();
    void tst_floatingWindowTitleBug();
    void tst_setFloatingSimple();

    void tst_resizeWindow_data();
    void tst_resizeWindow();
    void tst_restoreEmpty();
    void tst_restoreCentralFrame();
    void tst_shutdown();
    void tst_doubleClose();
    void tst_dockInternal();
    void tst_maximizeAndRestore();
    void tst_propagateResize2();
    void tst_28NestedWidgets();
    void tst_28NestedWidgets_data();
    void tst_negativeAnchorPosition2();
    void tst_negativeAnchorPosition3();
    void tst_negativeAnchorPosition4();
    void tst_negativeAnchorPosition5();
    void tst_startHidden();
    void tst_startHidden2();
    void tst_startClosed();
    void tst_closeReparentsToNull();
    void tst_invalidAnchorGroup();
    void tst_addAsPlaceholder();
    void tst_removeItem();
    void tst_clear();
    void tst_samePositionAfterHideRestore();
    void tst_dockDockWidgetNested();
    void tst_dockFloatingWindowNested();
    void tst_crash();
    void tst_refUnrefItem();
    void tst_placeholderCount();
    void tst_availableLengthForOrientation();
    void tst_closeShowWhenNoCentralFrame();
    void tst_setAstCurrentTab();
    void tst_placeholderDisappearsOnReadd();
    void tst_placeholdersAreRemovedProperly();
};

void TestCommon::tst_simple1()
{
    // Simply create a MainWindow
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    m->multiSplitter()->checkSanity();
}

void TestCommon::tst_simple2()
{
    // Simply create a MainWindow, and dock something on top
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dw = createDockWidget("dw", new MyWidget("dw", Qt::blue));
    auto fw = dw->floatingWindow();
    m->addDockWidget(dw, KDDockWidgets::Location_OnTop);
    m->multiSplitter()->checkSanity();
    delete fw;
}


void TestCommon::tst_doesntHaveNativeTitleBar()
{
    // Tests that a floating window doesn't have a native title bar
    // This test is mostly to test a bug that was happening with QtQuick, where the native title bar
    // would appear on linux
    EnsureTopLevelsDeleted e;

    auto dw1 = createDockWidget("dock1");
    FloatingWindow *fw = dw1->floatingWindow();
    QVERIFY(fw);
    QVERIFY(fw->windowFlags() & Qt::Tool);

#if defined(Q_OS_LINUX)
    QVERIFY(fw->windowFlags() & Qt::FramelessWindowHint);
#elif defined(Q_OS_WIN)
    QVERIFY(!(fw->windowFlags() & Qt::FramelessWindowHint));
#endif

    delete dw1->window();
}

void TestCommon::tst_resizeWindow2()
{
    // Tests that resizing the width of the main window will never move horizontal anchors

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1");
    auto dock2 = createDockWidget("2");

    FloatingWindow *fw1 = dock1->floatingWindow();
    FloatingWindow *fw2 = dock2->floatingWindow();
    m->addDockWidget(dock1, Location_OnTop);
    m->addDockWidget(dock2, Location_OnBottom);

    auto layout = m->multiSplitter();
    Separator *anchor = layout->separators().at(0);
    const int oldPosY = anchor->position();
    m->resize(QSize(m->width() + 10, m->height()));
    QCOMPARE(anchor->position(), oldPosY);
    layout->checkSanity();

    delete fw1;
    delete fw2;
}

void TestCommon::tst_hasLastDockedLocation()
{
    // Tests DockWidgetBase::hasPreviousDockedLocation()

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1");
    m->multiSplitter()->checkSanity();
    m->multiSplitter()->setObjectName("mainWindow-dropArea");
    dock1->floatingWindow()->multiSplitter()->setObjectName("first-dropArea1");
    dock1->floatingWindow()->multiSplitter()->checkSanity();
    auto window1 = dock1->window();
    QVERIFY(dock1->isFloating());
    QVERIFY(!dock1->hasPreviousDockedLocation());
    QVERIFY(dock1->setFloating(true));
    QVERIFY(!dock1->setFloating(false)); // No docking location, so it's not docked
    QVERIFY(dock1->isFloating());
    QVERIFY(!dock1->hasPreviousDockedLocation());

    m->addDockWidget(dock1, Location_OnBottom);
    m->multiSplitter()->checkSanity();

    QVERIFY(!dock1->isFloating());
    QVERIFY(dock1->setFloating(true));

    auto ms1 = dock1->floatingWindow()->multiSplitter();
    ms1->setObjectName("dropArea1");
    ms1->checkSanity();
    QVERIFY(dock1->hasPreviousDockedLocation());
    auto window11 = dock1->window();
    QVERIFY(dock1->setFloating(false));

    delete window1;
    delete window11;
}

void TestCommon::tst_ghostSeparator()
{
    // Tests a situation where a separator wouldn't be removed after a widget had been removed
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1");
    auto dock2 = createDockWidget("2");
    auto dock3 = createDockWidget("3");

    QPointer<FloatingWindow> fw1 = dock1->floatingWindow();
    QPointer<FloatingWindow> fw2 = dock2->floatingWindow();
    QPointer<FloatingWindow> fw3 = dock3->floatingWindow();

    dock1->addDockWidgetToContainingWindow(dock2, Location_OnRight);
    QCOMPARE(fw1->multiSplitter()->separators().size(), 1);
    QCOMPARE(Layouting::Separator::numSeparators(), 1);

    m->addDockWidget(dock3, Location_OnBottom);
    QCOMPARE(m->multiSplitter()->separators().size(), 0);
    QCOMPARE(Layouting::Separator::numSeparators(), 1);

    m->multiSplitter()->addMultiSplitter(fw1->multiSplitter(), Location_OnRight);
    QCOMPARE(m->multiSplitter()->separators().size(), 2);
    QCOMPARE(Layouting::Separator::numSeparators(), 2);

    delete fw1;
    delete fw2;
    delete fw3;
}

void TestCommon::tst_detachFromMainWindow()
{
    // Tests a situation where clicking the float button wouldn't work on QtQuick
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1");
    auto fw1 = dock1->window();
    m->addDockWidget(dock1, Location_OnTop);

    QVERIFY(m->multiSplitter()->mainWindow() != nullptr);
    QVERIFY(!dock1->isFloating());
    TitleBar *tb = dock1->titleBar();
    QVERIFY(tb == dock1->frame()->titleBar());
    QVERIFY(tb->isVisible());
    QVERIFY(!tb->isFloating());

    delete fw1;
}

void TestCommon::tst_detachPos()
{
    // Tests a situation where detaching a dock widget would send it to a bogus position
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1", new MyWidget(QStringLiteral("1"), Qt::black), {}, /** show = */false); // we're creating the dock widgets without showing them as floating initially, so it doesn't record the previous floating position
    auto dock2 = createDockWidget("2", new MyWidget(QStringLiteral("2"), Qt::black), {}, /** show = */false);

    QVERIFY(!dock1->isVisible());
    QVERIFY(!dock2->isVisible());

    m->addDockWidget(dock1, Location_OnLeft);
    m->addDockWidget(dock2, Location_OnRight);

    QVERIFY(!dock1->lastPositions().lastFloatingGeometry().isValid());
    QVERIFY(!dock2->lastPositions().lastFloatingGeometry().isValid());

    const int previousWidth = dock1->width();
    dock1->setFloating(true);
    QTest::qWait(400); // Needed for QtQuick

    QVERIFY(qAbs(previousWidth - dock1->width()) < 15); // 15px of difference when floating is fine, due to margins and what not.
    delete dock1->window();
}

void TestCommon::tst_floatingWindowSize()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1");
    auto fw1 = dock1->window();


    QTest::qWait(2000);

    QVERIFY(!fw1->geometry().isNull());
    QCOMPARE(fw1->size(), fw1->windowHandle()->size());

    delete fw1;
}

void TestCommon::tst_tabbingWithAffinities()
{
    EnsureTopLevelsDeleted e;
    // Tests that dock widgets with different affinities should not tab together

    auto m1 = createMainWindow(QSize(1000, 1000), MainWindowOption_None);
    m1->setAffinities({ "af1", "af2" });

    auto dw1 = new DockWidgetType("1");
    dw1->setAffinities({ "af1" });
    dw1->show();

    auto dw2 = new DockWidgetType("2");
    dw2->setAffinities({ "af2" });
    dw2->show();

    FloatingWindow *fw1 = dw1->floatingWindow();
    FloatingWindow *fw2 = dw2->floatingWindow();

    {
        SetExpectedWarning ignoreWarning("Refusing to dock widget with incompatible affinity");
        dw1->addDockWidgetAsTab(dw2);
        QVERIFY(dw1->window() != dw2->window());
    }

    m1->addDockWidget(dw1, Location_OnBottom);
    QVERIFY(!dw1->isFloating());

    {
        SetExpectedWarning ignoreWarning("Refusing to dock widget with incompatible affinity");
        auto dropArea = m1->dropArea();
        WindowBeingDragged wbd(fw2, fw2);
        QVERIFY(!dropArea->drop(&wbd, dw1->frame(), DropIndicatorOverlayInterface::DropLocation_Center));
        QVERIFY(dw1->window() != dw2->window());
    }

    delete fw1;
    delete fw2;
}

void TestCommon::tst_sizeAfterRedock()
{
    EnsureTopLevelsDeleted e;
    auto dw1 = new DockWidgetType(QStringLiteral("1"));
    auto dw2 = new DockWidgetType(QStringLiteral("2"));
    dw2->setWidget(new MyWidget("2", Qt::red));

    dw1->addDockWidgetToContainingWindow(dw2, Location_OnBottom);
    const int height2 = dw2->frame()->height();

    dw2->setFloating(true);
    QCOMPARE(height2, dw2->window()->height());
    auto oldFw2 = dw2->floatingWindow();

    // Redock
    FloatingWindow *fw1 = dw1->floatingWindow();
    DropArea *dropArea = fw1->dropArea();

    MultiSplitter *ms1 = fw1->multiSplitter();
    {
        WindowBeingDragged wbd2(oldFw2);
        const QRect suggestedDropRect = ms1->rectForDrop(&wbd2, Location_OnBottom, nullptr);
        QCOMPARE(suggestedDropRect.height(), height2);
    }

    dropArea->drop(dw2->floatingWindow(), Location_OnBottom, nullptr);

    QCOMPARE(dw2->frame()->height(), height2);

    delete dw1->window();
    delete oldFw2;
}

void TestCommon::tst_honourUserGeometry()
{
    EnsureTopLevelsDeleted e;
    auto m1 = createMainWindow(QSize(1000, 1000), MainWindowOption_None);
    auto dw1 = new DockWidgetType(QStringLiteral("1"));

    const QPoint pt(10, 10);
    dw1->move(pt);
    dw1->show();
    FloatingWindow *fw1 = dw1->floatingWindow();
    QCOMPARE(fw1->windowHandle()->geometry().topLeft(), pt);

    delete dw1->window();
}

void TestCommon::tst_floatingWindowTitleBug()
{
    // Test for #74
    EnsureTopLevelsDeleted e;
    auto dw1 = new DockWidgetType(QStringLiteral("1"));
    auto dw2 = new DockWidgetType(QStringLiteral("2"));
    auto dw3 = new DockWidgetType(QStringLiteral("3"));

    dw1->setObjectName(QStringLiteral("1"));
    dw2->setObjectName(QStringLiteral("2"));
    dw3->setObjectName(QStringLiteral("3"));

    dw1->show();
    dw1->addDockWidgetAsTab(dw2);
    dw1->addDockWidgetToContainingWindow(dw3, Location_OnBottom);

    dw1->titleBar()->onFloatClicked();

    QCOMPARE(dw3->titleBar()->title(), QLatin1String("3"));

    delete dw1->window();
    delete dw3->window();
}

void TestCommon::tst_resizeWindow_data()
{
    QTest::addColumn<bool>("doASaveRestore");
    QTest::newRow("false") << false;
    QTest::newRow("true") << true;
}

void TestCommon::tst_resizeWindow()
{
    QFETCH(bool, doASaveRestore);

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(501, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1", new MyWidget("1", Qt::red));
    auto dock2 = createDockWidget("2", new MyWidget("2", Qt::blue));
    QPointer<FloatingWindow> fw1 = dock1->floatingWindow();
    QPointer<FloatingWindow> fw2 = dock2->floatingWindow();
    m->addDockWidget(dock1, Location_OnLeft);
    m->addDockWidget(dock2, Location_OnRight);

    auto layout = m->multiSplitter();

    layout->checkSanity();

    const int oldWidth1 = dock1->width();
    const int oldWidth2 = dock2->width();

    QVERIFY(oldWidth2 - oldWidth1 <= 1); // They're not equal if separator thickness if even

    if (doASaveRestore) {
        LayoutSaver saver;
        saver.restoreLayout(saver.serializeLayout());
    }

    m->showMaximized();
    Testing::waitForResize(m.get());

    const int maximizedWidth1 = dock1->width();
    const int maximizedWidth2 = dock2->width();

    const double relativeDifference = qAbs((maximizedWidth1 - maximizedWidth2) / (1.0 * layout->width()));

    qDebug() << oldWidth1 << oldWidth2 << maximizedWidth1 << maximizedWidth2 << relativeDifference;
    QVERIFY(relativeDifference <= 0.01);

    m->showNormal();
    Testing::waitForResize(m.get());

    const int newWidth1 = dock1->width();
    const int newWidth2 = dock2->width();

    QCOMPARE(oldWidth1, newWidth1);
    QCOMPARE(oldWidth2, newWidth2);
    layout->checkSanity();

    delete fw1;
    delete fw2;
}

void TestCommon::tst_restoreEmpty()
{
    EnsureTopLevelsDeleted e;

    // Create an empty main window, save it to disk.
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto layout = m->multiSplitter();
    LayoutSaver saver;
    const QSize oldSize = m->size();
    QVERIFY(saver.saveToFile(QStringLiteral("layout_tst_restoreEmpty.json")));
    saver.restoreFromFile(QStringLiteral("layout_tst_restoreEmpty.json"));
    QVERIFY(m->multiSplitter()->checkSanity());
    QCOMPARE(layout->separators().size(), 0);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(m->size(), oldSize);
    QVERIFY(layout->checkSanity());
}

void TestCommon::tst_restoreCentralFrame()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500));
    auto layout = m->multiSplitter();

    QCOMPARE(layout->count(), 1);
    Item *item = m->dropArea()->centralFrame();
    QVERIFY(item);
    auto frame = static_cast<Frame *>(item->guestAsQObject());
    QCOMPARE(frame->options(), FrameOption_IsCentralFrame | FrameOption_AlwaysShowsTabs);
    QVERIFY(!frame->titleBar()->isVisible());

    LayoutSaver saver;
    QVERIFY(saver.saveToFile(QStringLiteral("layout_tst_restoreCentralFrame.json")));
    QVERIFY(saver.restoreFromFile(QStringLiteral("layout_tst_restoreCentralFrame.json")));

    QCOMPARE(layout->count(), 1);
    item = m->dropArea()->centralFrame();
    QVERIFY(item);
    frame = static_cast<Frame *>(item->guestAsQObject());
    QCOMPARE(frame->options(), FrameOption_IsCentralFrame | FrameOption_AlwaysShowsTabs);
    QVERIFY(!frame->titleBar()->isVisible());
}

void TestCommon::tst_setFloatingSimple()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dock1 = createDockWidget("dock1", new MyWidget("one"));
    m->addDockWidget(dock1, Location_OnTop);
    auto l = m->multiSplitter();
    dock1->setFloating(true);
    QVERIFY(l->checkSanity());
    dock1->setFloating(false);
    QVERIFY(l->checkSanity());
    dock1->setFloating(true);
    QVERIFY(l->checkSanity());
    dock1->setFloating(false);
    QVERIFY(l->checkSanity());
}

int main(int argc, char *argv[])
{
    if (!qpaPassedAsArgument(argc, argv)) {
        // Use offscreen by default as it's less annoying, doesn't create visible windows
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QApplication app(argc, argv);
    if (shouldSkipTests())
        return 0;

    TestCommon test;
    return QTest::qExec(&test, argc, argv);
}

void TestCommon::tst_doubleClose()
{
    EnsureTopLevelsDeleted e;
    {
        // Via close()
        auto dock1 = createDockWidget("hello", Qt::green);
        dock1->close();
        dock1->close();

        delete dock1->window();
    }
    {
        // Via the button
        auto dock1 = createDockWidget("hello", Qt::green);
        auto fw1 = dock1->floatingWindow();

        auto t = dock1->frame()->titleBar();
        t->onCloseClicked();
        t->onCloseClicked();

        delete dock1;
        delete fw1;
    }
}

void TestCommon::tst_dockInternal()
{
    /**
     * Here we dock relative to an existing widget, and not to the drop-area.
     */
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dropArea = m->dropArea();

    auto centralWidget = static_cast<Frame*>(dropArea->items()[0]->guestAsQObject());
    nestDockWidget(dock1, dropArea, centralWidget, KDDockWidgets::Location_OnRight);

    QVERIFY(dock1->width() < dropArea->width() - centralWidget->width());
}

void TestCommon::tst_maximizeAndRestore()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dock2 = createDockWidget("dock2", new QPushButton("two"));

    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    m->addDockWidget(dock2, KDDockWidgets::Location_OnRight);

    auto dropArea = m->dropArea();
    QVERIFY(dropArea->checkSanity());

    m->showMaximized();
    Testing::waitForResize(m.get());

    QVERIFY(dropArea->checkSanity());
    qDebug() << "About to show normal";
    m->showNormal();
    Testing::waitForResize(m.get());

    QVERIFY(dropArea->checkSanity());
}

void TestCommon::tst_propagateResize2()
{
    // |5|1|2|
    // | |3|4|

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dock2 = createDockWidget("dock2", new QPushButton("two"));
    m->addDockWidget(dock1, KDDockWidgets::Location_OnTop);
    m->addDockWidget(dock2, KDDockWidgets::Location_OnRight, dock1);

    auto dock3 = createDockWidget("dock3", new QPushButton("three"));
    auto dock4 = createDockWidget("dock4", new QPushButton("four"));

    m->addDockWidget(dock3, KDDockWidgets::Location_OnBottom);
    m->addDockWidget(dock4, KDDockWidgets::Location_OnRight, dock3);

    auto dock5 = createDockWidget("dock5", new QPushButton("five"));
    m->addDockWidget(dock5, KDDockWidgets::Location_OnLeft);

    auto dropArea = m->dropArea();
    dropArea->checkSanity();
}

void TestCommon::tst_shutdown()
{
    EnsureTopLevelsDeleted e;
    auto dock = createDockWidget("doc1", Qt::green);

    auto m = createMainWindow();
    m->show();
    QVERIFY(QTest::qWaitForWindowActive(m->windowHandle()));
    delete dock->window();
}

void TestCommon::tst_28NestedWidgets_data()
{
    QTest::addColumn<QVector<DockDescriptor>>("docksToCreate");
    QTest::addColumn<QVector<int>>("docksToHide");

    QVector<DockDescriptor> docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None }
    };

    QTest::newRow("28") << docks << QVector<int>{11, 0};

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnLeft, -1, nullptr, AddingOption_None },

    };

    QVector<int> docksToHide;
    for (int i = 0; i < docks.size(); ++i) {
        docksToHide << i;
    }

    QTest::newRow("anchor_intersection") << docks << docksToHide;

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
    };

    // 2. Produced valgrind invalid reads while adding
    QTest::newRow("valgrind") << docks << QVector<int>{};

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
    };
    QTest::newRow("bug_when_closing") << docks << QVector<int>{}; // Q_ASSERT(!isSquashed())

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
    };

    QTest::newRow("bug_when_closing2") << docks << QVector<int>{};    // Tests for void KDDockWidgets::Anchor::setPosition(int, KDDockWidgets::Anchor::SetPositionOptions) Negative position -69

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnBottom, 0, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None }
    };

    docksToHide.clear();
    for (int i = 0; i < 28; ++i) {
        if (i != 16 && i != 17 && i != 18 && i != 27)
            docksToHide << i;
    }

    QTest::newRow("bug_with_holes") << docks << docksToHide;

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnLeft, 17, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None } };

    docksToHide.clear();
    QTest::newRow("add_as_placeholder") << docks << docksToHide;

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden } };

    QTest::newRow("add_as_placeholder_simple") << docks << docksToHide;


    docks = {
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden } };

    docksToHide.clear();
    QTest::newRow("isSquashed_assert") << docks << docksToHide;

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_StartHidden },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden } };

    docksToHide.clear();
    QTest::newRow("negative_pos_warning") << docks << docksToHide;

    docks = {
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_None } };

    docksToHide.clear();
    QTest::newRow("bug") << docks << docksToHide;

    docks = {
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_None } };

    docksToHide.clear();
    QTest::newRow("bug2") << docks << docksToHide;

    docks = {
        {Location_OnLeft, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_StartHidden },
        {Location_OnTop, -1, nullptr, AddingOption_None },
        {Location_OnRight, -1, nullptr, AddingOption_None },
        {Location_OnLeft, -1, nullptr, AddingOption_None },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnRight, -1, nullptr, AddingOption_None } };

    docksToHide.clear();
    QTest::newRow("bug3") << docks << docksToHide;
}

void TestCommon::tst_28NestedWidgets()
{
    QFETCH(QVector<DockDescriptor>, docksToCreate);
    QFETCH(QVector<int>, docksToHide);

    // Tests a case that used to cause negative anchor position when turning into placeholder
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    int i = 0;
    for (DockDescriptor &desc : docksToCreate) {
        desc.createdDock = createDockWidget(QString("%1").arg(i), new QPushButton(QString("%1").arg(i).toLatin1()), {}, false);

        DockWidgetBase *relativeTo = nullptr;
        if (desc.relativeToIndex != -1)
            relativeTo = docksToCreate.at(desc.relativeToIndex).createdDock;
        m->addDockWidget(desc.createdDock, desc.loc, relativeTo, desc.option);
        QVERIFY(layout->checkSanity());
        ++i;
    }

    layout->checkSanity();

    // Run the saver in these complex scenarios:
    LayoutSaver saver;
    const QByteArray saved = saver.serializeLayout();
    QVERIFY(!saved.isEmpty());
    QVERIFY(saver.restoreLayout(saved));

    layout->checkSanity();

    for (int i : docksToHide) {
        docksToCreate.at(i).createdDock->close();
        layout->checkSanity();
        QTest::qWait(200);
    }

    layout->checkSanity();

    for (int i : docksToHide) {
        docksToCreate.at(i).createdDock->deleteLater();
        QVERIFY(Testing::waitForDeleted(docksToCreate.at(i).createdDock));
    }

    layout->checkSanity();

    // And hide the remaining ones
    i = 0;
    for (auto dock : docksToCreate) {
        if (dock.createdDock && dock.createdDock->isVisible()) {
            dock.createdDock->close();
            QTest::qWait(200); // Wait for the docks to be closed. TODO Replace with a global event filter and wait for any resize ?
        }
        ++i;
    }

    layout->checkSanity();

    // Cleanup
    for (auto dock : DockRegistry::self()->dockwidgets()) {
        dock->deleteLater();
        QVERIFY(Testing::waitForDeleted(dock));
    }
}

void TestCommon::tst_closeReparentsToNull()
{
    EnsureTopLevelsDeleted e;
    auto dock1 = createDockWidget("1", new QPushButton("1"));
    auto fw1 = dock1->window();
    QVERIFY(dock1->parent() != nullptr);
    dock1->close();
    QVERIFY(dock1->parent() == nullptr);
    delete fw1;
    delete dock1;
}

void TestCommon::tst_startHidden()
{
    // A really simple test for AddingOption_StartHidden

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1", new QPushButton("1"), {}, /*show=*/false);
    m->addDockWidget(dock1, Location_OnRight, nullptr, AddingOption_StartHidden);
    delete dock1;
}

void TestCommon::tst_startHidden2()
{
    EnsureTopLevelsDeleted e;
    {
        auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
        auto dock1 = createDockWidget("dock1", new QPushButton("one"), {}, false);
        auto dock2 = createDockWidget("dock2", new QPushButton("two"), {}, false);

        auto dropArea = m->dropArea();
        MultiSplitter *layout = dropArea;

        m->addDockWidget(dock1, Location_OnTop, nullptr, AddingOption_StartHidden);
        QVERIFY(layout->checkSanity());

        QCOMPARE(layout->count(), 1);
        QCOMPARE(layout->placeholderCount(), 1);

        m->addDockWidget(dock2, Location_OnTop);
        QVERIFY(layout->checkSanity());

        QCOMPARE(layout->count(), 2);
        QCOMPARE(layout->placeholderCount(), 1);

        qDebug() << dock1->isVisible();
        dock1->show();

        QCOMPARE(layout->count(), 2);
        QCOMPARE(layout->placeholderCount(), 0);

        Testing::waitForResize(dock2);
    }

    {
        auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
        auto dock1 = createDockWidget("dock1", new QPushButton("one"), {}, false);
        auto dock2 = createDockWidget("dock2", new QPushButton("two"), {}, false);
        auto dock3 = createDockWidget("dock3", new QPushButton("three"), {}, false);

        auto dropArea = m->dropArea();
        MultiSplitter *layout = dropArea;
        m->addDockWidget(dock1, Location_OnLeft, nullptr, AddingOption_StartHidden);

        m->addDockWidget(dock2, Location_OnBottom, nullptr, AddingOption_StartHidden);
        m->addDockWidget(dock3, Location_OnRight, nullptr, AddingOption_StartHidden);

        dock1->show();

        QCOMPARE(layout->count(), 3);
        QCOMPARE(layout->placeholderCount(), 2);

        dock2->show();
        dock3->show();
        Testing::waitForResize(dock2);
        layout->checkSanity();
    }
}

void TestCommon::tst_negativeAnchorPosition2()
{
    // Tests that the "Out of bounds position" warning doesn't appear. Test will abort if yes.
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    auto dock1 = createDockWidget("1", new QPushButton("1"), {}, /*show=*/false);
    auto dock2 = createDockWidget("2", new QPushButton("2"), {}, /*show=*/false);
    auto dock3 = createDockWidget("3", new QPushButton("3"), {}, /*show=*/false);

    m->addDockWidget(dock1, Location_OnLeft);
    m->addDockWidget(dock2, Location_OnRight, nullptr, AddingOption_StartHidden);
    m->addDockWidget(dock3, Location_OnRight);
    QCOMPARE(layout->placeholderCount(), 1);
    QCOMPARE(layout->count(), 3);

    dock1->setFloating(true);
    dock1->setFloating(false);
    dock2->deleteLater();
    layout->checkSanity();
    QVERIFY(Testing::waitForDeleted(dock2));
}

void TestCommon::tst_negativeAnchorPosition3()
{
    // 1. Another case, when floating a dock:
    EnsureTopLevelsDeleted e;
    QVector<DockDescriptor> docks = { {Location_OnLeft, -1, nullptr, AddingOption_None },
                                     {Location_OnRight, -1, nullptr, AddingOption_None },
                                     {Location_OnLeft, -1, nullptr, AddingOption_None },
                                     {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
                                     {Location_OnRight, -1, nullptr, AddingOption_None } };
    auto m = createMainWindow(docks);
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;
    layout->checkSanity();

    auto dock1 = docks.at(1).createdDock;
    auto dock3 = docks.at(3).createdDock;

    dock1->setFloating(true);
    delete dock1->window();
    delete dock3->window();

    layout->checkSanity();
}

void TestCommon::tst_negativeAnchorPosition4()
{
    // 1. Tests that we don't get a warning
    // Out of bounds position= -5 ; oldPosition= 0 KDDockWidgets::Anchor(0x55e726be9090, name = "left") KDDockWidgets::MainWindow(0x55e726beb8d0)
    EnsureTopLevelsDeleted e;
    QVector<DockDescriptor> docks = { { Location_OnLeft, -1, nullptr, AddingOption_StartHidden },
                                      { Location_OnTop, -1, nullptr, AddingOption_None },
                                      { Location_OnRight, -1, nullptr, AddingOption_None },
                                      { Location_OnLeft, -1, nullptr, AddingOption_None },
                                      { Location_OnRight, -1, nullptr, AddingOption_None } };

    auto m = createMainWindow(docks);
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;
    layout->checkSanity();

    auto dock1 = docks.at(1).createdDock;
    auto dock2 = docks.at(2).createdDock;
    dock2->setFloating(true);
    auto fw2 = dock2->floatingWindow();
    dropArea->addWidget(fw2->dropArea(), Location_OnLeft, dock1->frame());
    dock2->setFloating(true);
    fw2 = dock2->floatingWindow();

    dropArea->addWidget(fw2->dropArea(), Location_OnRight, dock1->frame());

    layout->checkSanity();
    docks.at(0).createdDock->deleteLater();
    docks.at(4).createdDock->deleteLater();
    Testing::waitForDeleted(docks.at(4).createdDock);
}

void TestCommon::tst_negativeAnchorPosition5()
{
    EnsureTopLevelsDeleted e;
    QVector<DockDescriptor> docks = {
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        {Location_OnBottom, -1, nullptr, AddingOption_StartHidden },
        };

    auto m = createMainWindow(docks);
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;
    layout->checkSanity();

    auto dock0 = docks.at(0).createdDock;
    auto dock1 = docks.at(1).createdDock;

    dock1->show();
    QVERIFY(layout->checkSanity());
    dock0->show();
    QVERIFY(layout->checkSanity());

    // Cleanup
    for (auto dock : DockRegistry::self()->dockwidgets())
        dock->deleteLater();

    QVERIFY(Testing::waitForDeleted(dock0));
}

void TestCommon::tst_invalidAnchorGroup()
{
    // Tests a bug I got. Should not warn.
    EnsureTopLevelsDeleted e;

    {
        auto dock1 = createDockWidget("dock1", new QPushButton("one"));
        auto dock2 = createDockWidget("dock2", new QPushButton("two"));

        QPointer<FloatingWindow> fw = dock2->morphIntoFloatingWindow();
        nestDockWidget(dock1, fw->dropArea(), nullptr, KDDockWidgets::Location_OnTop);

        dock1->close();
        Testing::waitForResize(dock2);
        auto layout = fw->dropArea();
        layout->checkSanity();

        dock2->close();
        dock1->deleteLater();
        dock2->deleteLater();
        Testing::waitForDeleted(dock1);
    }

    {
        // Stack 1, 2, 3, close 2, close 1

        auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
        auto dock1 = createDockWidget("dock1", new QPushButton("one"));
        auto dock2 = createDockWidget("dock2", new QPushButton("two"));
        auto dock3 = createDockWidget("dock3", new QPushButton("three"));

        m->addDockWidget(dock3, Location_OnTop);
        m->addDockWidget(dock2, Location_OnTop);
        m->addDockWidget(dock1, Location_OnTop);

        dock2->close();
        dock1->close();

        dock1->deleteLater();
        dock2->deleteLater();
        Testing::waitForDeleted(dock1);
    }
}

void TestCommon::tst_addAsPlaceholder()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("dock1", new QPushButton("one"), {}, false);
    auto dock2 = createDockWidget("dock2", new QPushButton("two"), {}, false);

    m->addDockWidget(dock1, Location_OnBottom);
    m->addDockWidget(dock2, Location_OnTop, nullptr, AddingOption_StartHidden);

    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);
    QVERIFY(!dock2->isVisible());

    dock2->show();
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 0);

    layout->checkSanity();

    // Cleanup
    dock2->deleteLater();
    Testing::waitForDeleted(dock2);
}

void TestCommon::tst_removeItem()
{
    // Tests that MultiSplitterLayout::removeItem() works
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dock2 = createDockWidget("dock2", new QPushButton("two"), {}, false);
    auto dock3 = createDockWidget("dock3", new QPushButton("three"));

    m->addDockWidget(dock1, Location_OnBottom);
    m->addDockWidget(dock2, Location_OnTop, nullptr, AddingOption_StartHidden);
    Item *item2 = dock2->lastPositions().lastItem();

    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);

    // 1. Remove an item that's a placeholder

    layout->removeItem(item2);

    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    // 2. Remove an item that has an actual widget
    Item *item1 = dock1->lastPositions().lastItem();
    layout->removeItem(item1);
    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->placeholderCount(), 0);

    // 3. Remove an item that has anchors following one of its other anchors (Tests that anchors stop following)
    // Stack 1, 2, 3
    m->addDockWidget(dock3, Location_OnBottom);
    m->addDockWidget(dock2, Location_OnBottom);
    m->addDockWidget(dock1, Location_OnBottom);
    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->placeholderCount(), 0);

    dock2->close();
    auto frame1 = dock1->frame();
    dock1->close();
    QVERIFY(Testing::waitForDeleted(frame1));

    QCOMPARE(layout->count(), 3);
    QCOMPARE(layout->placeholderCount(), 2);

    // Now remove the items
    layout->removeItem(dock2->lastPositions().lastItem());
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);
    layout->checkSanity();
    layout->removeItem(dock1->lastPositions().lastItem());
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    // Add again
    m->addDockWidget(dock2, Location_OnBottom);
    m->addDockWidget(dock1, Location_OnBottom);
    dock2->close();
    frame1 = dock1->frame();
    dock1->close();
    QVERIFY(Testing::waitForDeleted(frame1));

    // Now remove the items, but first dock1
    layout->removeItem(dock1->lastPositions().lastItem());
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);
    layout->checkSanity();
    layout->removeItem(dock2->lastPositions().lastItem());
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);
    layout->checkSanity();

    // Add again, stacked as 1, 2, 3, then close 2 and 3.
    m->addDockWidget(dock2, Location_OnTop);
    m->addDockWidget(dock1, Location_OnTop);

    auto frame2 = dock2->frame();
    dock2->close();
    Testing::waitForDeleted(frame2);

    auto frame3 = dock3->frame();
    dock3->close();
    Testing::waitForDeleted(frame3);

    // The second anchor is now following the 3rd, while the 3rd is following 'bottom'
    layout->removeItem(dock3->lastPositions().lastItem()); // will trigger the 3rd anchor to be removed
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);
    layout->checkSanity();

    dock1->deleteLater();
    dock2->deleteLater();
    dock3->deleteLater();
    Testing::waitForDeleted(dock3);
}

void TestCommon::tst_clear()
{
    // Tests MultiSplitterLayout::clear()
    EnsureTopLevelsDeleted e;
    QCOMPARE(Frame::dbg_numFrames(), 0);

    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1", new QPushButton("1"));
    auto dock2 = createDockWidget("2", new QPushButton("2"));
    auto dock3 = createDockWidget("3", new QPushButton("3"));
    auto fw3 = dock3->floatingWindow();

    m->addDockWidget(dock1, Location_OnLeft);
    m->addDockWidget(dock2, Location_OnRight);
    m->addDockWidget(dock3, Location_OnRight);
    QVERIFY(Testing::waitForDeleted(fw3));
    dock3->close();

    QCOMPARE(Frame::dbg_numFrames(), 3);

    auto layout = m->multiSplitter();
    layout->rootItem()->clear();

    QCOMPARE(layout->count(), 0);
    QCOMPARE(layout->placeholderCount(), 0);
    layout->checkSanity();

    // Cleanup
    dock3->deleteLater();
    QVERIFY(Testing::waitForDeleted(dock3));
}

void TestCommon::tst_samePositionAfterHideRestore()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("1", new QPushButton("1"));
    auto dock2 = createDockWidget("2", new QPushButton("2"));
    auto dock3 = createDockWidget("3", new QPushButton("3"));

    m->addDockWidget(dock1, Location_OnLeft);
    m->addDockWidget(dock2, Location_OnRight);
    m->addDockWidget(dock3, Location_OnRight);
    QRect geo2 = dock2->frame()->QWidgetAdapter::geometry();
    dock2->setFloating(true);

    auto fw2 = dock2->floatingWindow();
    dock2->setFloating(false);
    QVERIFY(Testing::waitForDeleted(fw2));
    QCOMPARE(geo2, dock2->frame()->QWidgetAdapter::geometry());
    m->multiSplitter()->checkSanity();
}

void TestCommon::tst_startClosed()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dock2 = createDockWidget("dock2", new QPushButton("two"));

    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    m->addDockWidget(dock1, Location_OnTop);
    Frame *frame1 = dock1->frame();
    dock1->close();
    Testing::waitForDeleted(frame1);

    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 1);

    m->addDockWidget(dock2, Location_OnTop);

    layout->checkSanity();

    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);

    dock1->show();
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 0);
}

void TestCommon::tst_dockDockWidgetNested()
{
    EnsureTopLevelsDeleted e;
    // Test detaching too, and check if the window size is correct
    // TODO
}

void TestCommon::tst_dockFloatingWindowNested()
{
    EnsureTopLevelsDeleted e;
    // TODO
}

void TestCommon::tst_crash()
{
     // tests some crash I got

    EnsureTopLevelsDeleted e;

    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None);
    auto dock1 = createDockWidget("dock1", new QPushButton("one"));
    auto dock2 = createDockWidget("dock2", new QPushButton("two"));
    auto layout = m->multiSplitter();

    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    Item *item1 = layout->itemForFrame(dock1->frame());
    dock1->addDockWidgetAsTab(dock2);

    QVERIFY(!dock1->isFloating());
    dock1->setFloating(true);
    QVERIFY(dock1->isFloating());
    QVERIFY(!dock1->isInMainWindow());

    Item *layoutItem = dock1->lastPositions().lastItem();
    QVERIFY(layoutItem && DockRegistry::self()->itemIsInMainWindow(layoutItem));
    QCOMPARE(layoutItem, item1);

    QCOMPARE(layout->placeholderCount(), 0);
    QCOMPARE(layout->count(), 1);

    // Move from tab to bottom
    m->addDockWidget(dock2, KDDockWidgets::Location_OnBottom);

    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);

    delete dock1->window();
}

void TestCommon::tst_refUnrefItem()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow();
    auto dock1 = createDockWidget("dock1", new QPushButton("1"));
    auto dock2 = createDockWidget("dock2", new QPushButton("2"));
    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    m->addDockWidget(dock2, KDDockWidgets::Location_OnRight);
    auto dropArea = m->dropArea();
    auto layout = dropArea;
    QPointer<Frame> frame1 = dock1->frame();
    QPointer<Frame> frame2 = dock2->frame();
    QPointer<Item> item1 = layout->itemForFrame(frame1);
    QPointer<Item> item2 = layout->itemForFrame(frame2);
    QVERIFY(item1.data());
    QVERIFY(item2.data());
    QCOMPARE(item1->refCount(), 2); // 2 - the item and its frame, which can be persistent
    QCOMPARE(item2->refCount(), 2);

    // 1. Delete a dock widget directly. It should delete its frame and also the Item
    delete dock1;
    Testing::waitForDeleted(frame1);
    QVERIFY(!frame1.data());
    QVERIFY(!item1.data());

    // 2. Delete dock3, but neither the frame or the item is deleted, since there were two tabs to begin with
    auto dock3 = createDockWidget("dock3", new QPushButton("3"));
    QCOMPARE(item2->refCount(), 2);
    dock2->addDockWidgetAsTab(dock3);
    QCOMPARE(item2->refCount(), 3);
    delete dock3;
    QVERIFY(item2.data());
    QCOMPARE(frame2->dockWidgets().size(), 1);

    // 3. Close dock2. frame2 should be deleted, but item2 preserved.
    QCOMPARE(item2->refCount(), 2);
    dock2->close();
    Testing::waitForDeleted(frame2);
    QVERIFY(dock2);
    QVERIFY(item2.data());
    QCOMPARE(item2->refCount(), 1);
    QCOMPARE(dock2->lastPositions().lastItem(), item2.data());
    delete dock2;

    QVERIFY(!item2.data());
    QCOMPARE(layout->count(), 1);

    // 4. Move a closed dock widget from one mainwindow to another
    // It should delete its old placeholder
    auto dock4 = createDockWidget("dock4", new QPushButton("4"));
    m->addDockWidget(dock4, KDDockWidgets::Location_OnLeft);

    QPointer<Frame> frame4 = dock4->frame();
    QPointer<Item> item4 = layout->itemForFrame(frame4);
    dock4->close();
    Testing::waitForDeleted(frame4);
    QCOMPARE(item4->refCount(), 1);
    QVERIFY(item4->isPlaceholder());
    layout->checkSanity();

    auto m2 = createMainWindow();
    m2->addDockWidget(dock4, KDDockWidgets::Location_OnLeft);
    m2->multiSplitter()->checkSanity();
    QVERIFY(!item4.data());
}

void TestCommon::tst_placeholderCount()
{
    EnsureTopLevelsDeleted e;
    // Tests MultiSplitterLayout::count(),visibleCount() and placeholdercount()

    // 1. MainWindow with just the initial frame.
    auto m = createMainWindow();
    auto dock1 = createDockWidget("1", new QPushButton("1"));
    auto dock2 = createDockWidget("2", new QPushButton("2"));
    auto dropArea = m->dropArea();
    auto layout = dropArea;

    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->visibleCount(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    // 2. MainWindow with central frame and left widget
    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->visibleCount(), 2);
    QCOMPARE(layout->placeholderCount(), 0);

    // 3. Add another dockwidget, this time tabbed in the center. It won't increase count, as it reuses an existing frame.
    m->addDockWidgetAsTab(dock2);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->visibleCount(), 2);
    QCOMPARE(layout->placeholderCount(), 0);

    // 4. Float dock1. It should create a placeholder
    dock1->setFloating(true);

    auto fw = dock1->floatingWindow();

    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->visibleCount(), 1);
    QCOMPARE(layout->placeholderCount(), 1);

    // 5. Re-dock dock1. It should reuse the placeholder
    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->visibleCount(), 2);
    QCOMPARE(layout->placeholderCount(), 0);

    // 6. Again
    dock1->setFloating(true);
    fw = dock1->floatingWindow();
    m->addDockWidget(dock1, KDDockWidgets::Location_OnLeft);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->visibleCount(), 2);
    QCOMPARE(layout->placeholderCount(), 0);
    layout->checkSanity();

    Testing::waitForDeleted(fw);
}

void TestCommon::tst_availableLengthForOrientation()
{
    EnsureTopLevelsDeleted e;

    // 1. Test a completely empty window, it's available space is its size minus the static separators thickness
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None); // Remove central frame
    auto dropArea = m->dropArea();
    MultiSplitter *layout = dropArea;

    int availableWidth = layout->availableLengthForOrientation(Qt::Horizontal);
    int availableHeight = layout->availableLengthForOrientation(Qt::Vertical);
    QCOMPARE(availableWidth, layout->width());
    QCOMPARE(availableHeight, layout->height());

    //2. Now do the same, but we have some widget docked

    auto dock1 = createDockWidget("dock1", new QPushButton("1"));
    m->addDockWidget(dock1, Location_OnLeft);

    const int dock1MinWidth = layout->itemForFrame(dock1->frame())->minLength(Qt::Horizontal);
    const int dock1MinHeight = layout->itemForFrame(dock1->frame())->minLength(Qt::Vertical);

    availableWidth = layout->availableLengthForOrientation(Qt::Horizontal);
    availableHeight = layout->availableLengthForOrientation(Qt::Vertical);
    QCOMPARE(availableWidth, layout->width() - dock1MinWidth);
    QCOMPARE(availableHeight, layout->height() - dock1MinHeight);
    m->multiSplitter()->checkSanity();
}

void TestCommon::tst_closeShowWhenNoCentralFrame()
{
    EnsureTopLevelsDeleted e;
    // Tests a crash I got when hidding and showing and no central frame

    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None); // Remove central frame
    QPointer<DockWidgetBase> dock1 = createDockWidget("1", new QPushButton("1"));
    m->addDockWidget(dock1, Location_OnLeft);
    dock1->close();
    m->multiSplitter()->checkSanity();

    QVERIFY(!dock1->frame());
    QVERIFY(!Testing::waitForDeleted(dock1)); // It was being deleted due to a bug
    QVERIFY(dock1);
    dock1->show();
    m->multiSplitter()->checkSanity();
}

void TestCommon::tst_setAstCurrentTab()
{
    EnsureTopLevelsDeleted e;

    // Tests DockWidget::setAsCurrentTab() and DockWidget::isCurrentTab()
    // 1. a single dock widget is current, by definition
    auto dock1 = createDockWidget("1", new QPushButton("1"));
    QVERIFY(dock1->isCurrentTab());

    // 2. Tab dock2 to the group, dock2 is current now
    auto dock2 = createDockWidget("2", new QPushButton("2"));
    dock1->addDockWidgetAsTab(dock2);
    QVERIFY(!dock1->isCurrentTab());
    QVERIFY(dock2->isCurrentTab());

    // 3. Set dock1 as current
    dock1->setAsCurrentTab();
    QVERIFY(dock1->isCurrentTab());
    QVERIFY(!dock2->isCurrentTab());

    auto fw = dock1->floatingWindow();
    QVERIFY(fw);
    fw->multiSplitter()->checkSanity();

    delete dock1; delete dock2;
    Testing::waitForDeleted(fw);
}

void TestCommon::tst_placeholderDisappearsOnReadd()
{
    // This tests that addMultiSplitter also updates refcount of placeholders

    // 1. Detach a widget and dock it on the opposite side. Placeholder
    // should have been deleted and anchors properly positioned

    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None); // Remove central frame
    MultiSplitter *layout = m->multiSplitter();

    QPointer<DockWidgetBase> dock1 = createDockWidget("1", new QPushButton("1"));
    m->addDockWidget(dock1, Location_OnLeft);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    dock1->setFloating(true);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 1);

    dock1->morphIntoFloatingWindow();
    auto fw = dock1->floatingWindow();
    layout->addMultiSplitter(fw->dropArea(), Location_OnRight );

    QCOMPARE(layout->placeholderCount(), 0);
    QCOMPARE(layout->count(), 1);

    layout->checkSanity();
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    // The dock1 should occupy the entire width
    QCOMPARE(dock1->frame()->width(), layout->width());

    QVERIFY(Testing::waitForDeleted(fw));
}

void TestCommon::tst_placeholdersAreRemovedProperly()
{
    EnsureTopLevelsDeleted e;
    auto m = createMainWindow(QSize(800, 500), MainWindowOption_None); // Remove central frame
    MultiSplitter *layout = m->multiSplitter();
    QPointer<DockWidgetBase> dock1 = createDockWidget("1", new QPushButton("1"));
    QPointer<DockWidgetBase> dock2 = createDockWidget("2", new QPushButton("2"));
    m->addDockWidget(dock1, Location_OnLeft);
    Item *item = layout->items().constFirst();
    m->addDockWidget(dock2, Location_OnRight);
    QVERIFY(!item->isPlaceholder());
    dock1->setFloating(true);
    QVERIFY(item->isPlaceholder());

    QCOMPARE(layout->separators().size(), 0);
    QCOMPARE(layout->count(), 2);
    QCOMPARE(layout->placeholderCount(), 1);
    layout->removeItem(item);
    QCOMPARE(layout->separators().size(), 0);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);

    // 2. Recreate the placeholder. This time delete the dock widget to see if placeholder is deleted too.
    m->addDockWidget(dock1, Location_OnLeft);
    dock1->setFloating(true);
    auto window1 = Tests::make_qpointer(dock1->window());
    delete dock1;
    QCOMPARE(layout->separators().size(), 0);
    QCOMPARE(layout->count(), 1);
    QCOMPARE(layout->placeholderCount(), 0);
    layout->checkSanity();

    delete window1;
}

#include "tst_common.moc"
