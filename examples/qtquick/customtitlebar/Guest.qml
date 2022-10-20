/*
  This file is part of KDDockWidgets.

  SPDX-FileCopyrightText: 2020-2022 Klarälvdalens Datakonsult AB, a KDAB Group company <info@kdab.com>
  Author: Sergio Martins <sergio.martins@kdab.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only

  Contact KDAB at <info@kdab.com> for commercial licensing options.
*/

import QtQuick 2.9
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 1.5 as QQC1

import com.kdab.dockwidgets 2.0 as KDDW

Item {
    anchors.fill: parent

    property var background
    property var logo

	KDDW.LayoutSaver {
		id: layoutSaver
	}

	Rectangle {
		anchors.fill: parent
		color: "green"

		ColumnLayout {
			Button {
				text: "Save"
				onClicked: {
					layoutSaver.saveToFile ("/tmp/layout");
				}
			}

			Button {
				text: "Restore"
				onClicked: {
					layoutSaver.restoreFromFile ("/tmp/layout");
				}
			}
		}
	}
}
