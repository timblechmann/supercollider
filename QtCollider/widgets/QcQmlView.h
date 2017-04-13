/************************************************************************
*
* Copyright 2016 Tim Blechmann
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/

#include "../QcHelper.h"
#include "../style/style.hpp"

#include <QtQuickWidgets/QQuickWidget>

#include <QtQml/QQmlComponent>
#include <QtQuick/QQuickItem>


class QcQmlView : public QQuickWidget, QcHelper
{
  Q_OBJECT
  Q_PROPERTY( QString source READ getSource WRITE setSource NOTIFY sourceChanged )

public:
  QcQmlView();

  QString getSource() { return mSource; }

  void setSource( QString source );

  Q_INVOKABLE QVariant getQMLProperty( QString propertyName );
  Q_INVOKABLE void     setQMLProperty( QString propertyName, QString value );
  Q_INVOKABLE void     setQMLProperty( QString propertyName, float value );
  Q_INVOKABLE void     setQMLProperty( QString propertyName, int value );

Q_SIGNALS:
  void sourceChanged( QString newSource );

private:
  void onComponentReady();
  void onComponentLoadingFailed();

  QString         mSource;
  QQmlComponent * mComponent = nullptr;
  QQuickItem *    mRootItem  = nullptr;
};
