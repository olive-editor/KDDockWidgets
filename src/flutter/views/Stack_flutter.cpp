/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2022 Klarälvdalens Datakonsult AB, a KDAB Group company
  <info@kdab.com> Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

#include "Stack_flutter.h"
#include "Controller.h"
#include "kddockwidgets/controllers/Stack.h"
#include "kddockwidgets/controllers/TitleBar.h"
#include "flutter/ViewFactory_flutter.h"
#include "DockRegistry.h"
#include "Config.h"
#include "Window.h"
#include "private/View_p.h"

#include <QMouseEvent>
#include <QTabBar>
#include <QHBoxLayout>
#include <QAbstractButton>
#include <QMenu>

using namespace KDDockWidgets;
using namespace KDDockWidgets::Views;

Stack_flutter::Stack_flutter(Controllers::Stack *controller, View *parent)
    : View_flutter(controller, Type::Stack, parent)
    , StackViewInterface(controller)
{
}

void Stack_flutter::init()
{
}

int Stack_flutter::numDockWidgets() const
{
    return 0;
}

void Stack_flutter::removeDockWidget(Controllers::DockWidget *)
{
}

int Stack_flutter::indexOfDockWidget(const Controllers::DockWidget *) const
{
    return 0;
}

void Stack_flutter::setCurrentDockWidget(int index)
{
    Q_UNUSED(index);
}

bool Stack_flutter::insertDockWidget(int index, Controllers::DockWidget *dw, const QIcon &icon,
                                     const QString &title)
{
    Q_UNUSED(index);
    Q_UNUSED(dw);
    Q_UNUSED(icon);
    Q_UNUSED(title);
    return true;
}

void Stack_flutter::renameTab(int index, const QString &text)
{
    Q_UNUSED(index);
    Q_UNUSED(text);
}

void Stack_flutter::changeTabIcon(int index, const QIcon &icon)
{
    Q_UNUSED(index);
    Q_UNUSED(icon);
}

Controllers::DockWidget *Stack_flutter::dockwidgetAt(int index) const
{
    Q_UNUSED(index);
    return nullptr;
}

int Stack_flutter::currentIndex() const
{
    return 0;
}

void Stack_flutter::setDocumentMode(bool)
{
}

bool Stack_flutter::isPositionDraggable(QPoint) const
{
    return true;
}