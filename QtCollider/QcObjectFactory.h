/************************************************************************
*
* Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
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

#ifndef QC_OBJECT_FACTORY_H
#define QC_OBJECT_FACTORY_H

#include "QObjectProxy.h"
#include "Common.h"
#include "Slot.h"

#include <PyrObject.h>

#include <QMetaObject>
#include <QObject>
#include <QWidget>
#include <QHash>

class QcAbstractFactory;

typedef QHash<QString,QcAbstractFactory*> QcFactoryHash;

namespace QtCollider {
  QcFactoryHash & factories ();
}

class QcAbstractFactory
{
public:
  QcAbstractFactory( const char *className ) {
    qcDebugMsg( 2, QString("Declaring class '%1'").arg(className) );
    QtCollider::factories().insert( className, this );
  }
  virtual const QMetaObject *metaObject() = 0;
  virtual QObjectProxy *newInstance( PyrObject *, QtCollider::Variant arg[10] ) = 0;
};


template <class QOBJECT> class QcObjectFactory : public QcAbstractFactory
{
public:
  QcObjectFactory() : QcAbstractFactory( QOBJECT::staticMetaObject.className() ) {}

  const QMetaObject *metaObject() {
    return &QOBJECT::staticMetaObject;
  }

  virtual QObjectProxy *newInstance( PyrObject *scObject, QtCollider::Variant arg[10] ) {
    QOBJECT *qObject;

    if( arg[0].type() == QMetaType::Void ) {
      qObject = new QOBJECT;
    }
    else {
      QObject *obj = QOBJECT::staticMetaObject.newInstance(
        arg[0].asGenericArgument(),
        arg[1].asGenericArgument(),
        arg[2].asGenericArgument(),
        arg[3].asGenericArgument(),
        arg[4].asGenericArgument(),
        arg[5].asGenericArgument(),
        arg[6].asGenericArgument(),
        arg[7].asGenericArgument(),
        arg[8].asGenericArgument(),
        arg[9].asGenericArgument()
      );

      qObject = qobject_cast<QOBJECT*>(obj);
      if( !qObject ) {
        qcErrorMsg( QString("No appropriate constructor found for '%1'.")
          .arg( QOBJECT::staticMetaObject.className() ) );
        return 0;
      }
    }

    return proxy( qObject, scObject );
  }

protected:

  virtual QObjectProxy *proxy( QOBJECT *obj, PyrObject *sc_obj ) {
    QObjectProxy *prox( new QObjectProxy(obj, sc_obj) );
    initialize( prox, obj );
    return prox;
  }

  virtual void initialize( QObjectProxy *proxy, QOBJECT *obj ) {};
};



#endif //QC_OBJECT_FACTORY_H
