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

#include "QcQmlView.h"
#include <QQmlComponent>
#include <QQmlEngine>
#include "../QcWidgetFactory.h"

#include <QtCore/QUrl>

#include <QDebug>

QC_DECLARE_QWIDGET_FACTORY(QcQmlView)

QcQmlView::QcQmlView()
{
  setAttribute(Qt::WA_AcceptTouchEvents);

  setClearColor( Qt::green );
  setResizeMode( QQuickWidget::SizeRootObjectToView );

  connect( this, &QQuickWidget::sceneGraphError, [](QQuickWindow::SceneGraphError error, const QString &message) {
    qWarning() << message;
  } );

  connect( this, &QQuickWidget::statusChanged, [](QQuickWidget::Status status) {
    qWarning() << status;
  } );

  mComponent = new QQmlComponent( engine(), this );
}

void QcQmlView::setSource(QString source)
{
  if( source == mSource )
    return;

  QUrl sourceAsUrl( source );

  // source can either be an url or a qml string
  if( sourceAsUrl.isValid() ) {
    mComponent->loadUrl( sourceAsUrl );
  } else {
    mComponent->setData( source.toUtf8(), QUrl() );
  }

  switch( mComponent->status() ) {

  case QQmlComponent::Ready:
    onComponentReady();
    return;

  case QQmlComponent::Error:
    onComponentLoadingFailed();
    return;

  case QQmlComponent::Loading:
    connect( mComponent, &QQmlComponent::statusChanged, [this]( QQmlComponent::Status status ) {
      switch( status ) {
      case QQmlComponent::Ready:
        onComponentReady();
        return;

      case QQmlComponent::Error:
        onComponentLoadingFailed();
        return;

      default:
        return;
      }

    } );

    return;
  }

}

void QcQmlView::onComponentReady()
{
  delete mRootItem;
  mRootItem = qobject_cast<QQuickItem *>( mComponent->create() );

  mRootItem->setParentItem( quickWindow()->contentItem() );
  mRootItem->setParent( this );
}

void QcQmlView::onComponentLoadingFailed()
{
  if( mComponent->status() == QQmlComponent::Error ) {
    for( auto error : mComponent->errors() )
      qWarning() << error;

    quickWindow()->setColor( Qt::red );
  }
}


QVariant QcQmlView::getQMLProperty(QString propertyName)
{
  if( !mRootItem )
    return QVariant();

  return mRootItem->property( propertyName.toStdString().c_str() );
}

void QcQmlView::setQMLProperty( QString propertyName, QString value )
{
  if( !mRootItem )
    return;

  mRootItem->setProperty( propertyName.toStdString().c_str(), value );
}

void QcQmlView::setQMLProperty(QString propertyName, float value )
{
  if( !mRootItem )
    return;

  mRootItem->setProperty( propertyName.toStdString().c_str(), value );
}

void QcQmlView::setQMLProperty(QString propertyName, int value )
{
  if( !mRootItem )
    return;

  mRootItem->setProperty( propertyName.toStdString().c_str(), value );
}
