/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2019-2020 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sérgio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

/**
 * @file
 * @brief The QQuickItem counter part of TabWidgetQuick. Handles GUI while TabWidget handles state.
 *
 * @author Sérgio Martins \<sergio.martins@kdab.com\>
 */

#ifndef KDTABWIDGETQUICK_P_H
#define KDTABWIDGETQUICK_P_H

#include "TabWidget_p.h"
#include "QWidgetAdapter_quick_p.h"

#include <QQuickItem>
#include <QAbstractListModel>

namespace KDDockWidgets {

class Frame;
class TabBar;
class DockWidgetModel;

class DOCKS_EXPORT TabWidgetQuick
    : public QWidgetAdapter
    , public TabWidget
{
    Q_OBJECT
public:
    explicit TabWidgetQuick(Frame *parent);

    TabBar *tabBar() const override;

    int numDockWidgets() const override;
    void removeDockWidget(DockWidgetBase *) override;
    int indexOfDockWidget(DockWidgetBase *) const override;
    DockWidgetModel *dockWidgetModel() const;
    DockWidgetBase *dockwidgetAt(int index) const override;
    int currentIndex() const override;
    bool insertDockWidget(int index, DockWidgetBase *, const QIcon&, const QString &title) override;
    void setCurrentDockWidget(int index) override;

protected:
    bool isPositionDraggable(QPoint p) const override;
    void setTabBarAutoHide(bool) override;
    void renameTab(int index, const QString &) override;

Q_SIGNALS:
    void currentDockWidgetChanged(DockWidgetBase *dw);
    void countChanged();

private:
    Q_DISABLE_COPY(TabWidgetQuick)
    DockWidgetModel *const m_dockWidgetModel;
    TabBar *const m_tabBar;
    DockWidgetBase *m_currentDockWidget = nullptr;
};

class DockWidgetModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        Role_Title = Qt::UserRole
    };

    explicit DockWidgetModel(QObject *parent);
    int count() const;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    DockWidgetBase *dockWidgetAt(int index) const;
    void remove(DockWidgetBase *);
    int indexOf(DockWidgetBase *);
    bool insert(DockWidgetBase *dw, int index);
    bool contains(DockWidgetBase *dw) const;
protected:
    QHash<int, QByteArray> roleNames() const override;
Q_SIGNALS:
    void countChanged();
private:
    void emitDataChangedFor(DockWidgetBase *);
    DockWidgetBase::List m_dockWidgets;
    QHash<DockWidgetBase *, QVector<QMetaObject::Connection> > m_connections; // To make it easy to disconnect from lambdas
};


}

#endif