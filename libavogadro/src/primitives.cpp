/**********************************************************************
  Primitives - Wrapper class around the OpenBabel classes

  Copyright (C) 2006 by Geoffrey R. Hutchison
  Copyright (C) 2006 by Donald Ephraim Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Some code is based on Open Babel
  For more information, see <http://openbabel.sourceforge.net/>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
***********************************************************************/

#include "config.h"

#include <avogadro/primitives.h>
#include <QDebug>
#include <eigen/regression.h>

namespace Avogadro {

  using namespace OpenBabel;

  class PrimitivePrivate {
    public:
      PrimitivePrivate() : type(Primitive::OtherType), selected(false) {};
      
      enum Primitive::Type type;
      bool selected;
  };

  Primitive::Primitive(QObject *parent) : d(new PrimitivePrivate), QObject(parent) {}
  
  Primitive::Primitive(enum Type type, QObject *parent) : d(new PrimitivePrivate), QObject(parent)
  {
    d->type = type;
  }
  
  Primitive::~Primitive()
  {
    delete d;
  }
  
  bool Primitive::isSelected() const
  {
    return d->selected;
  }
  
  void Primitive::setSelected( bool s ) 
  {
    d->selected = s;
  }
  
  void Primitive::toggleSelected()
  {
    d->selected = !d->selected;
  }
  
  enum Primitive::Type Primitive::type() const
  {
    return d->type;
  }

  void Primitive::update()
  {
    emit updated();
  }
  
  Molecule::Molecule(QObject *parent) : OpenBabel::OBMol(), Primitive(MoleculeType, parent) 
  {
    connect(this, SIGNAL(updated()), this, SLOT(updatePrimitive()));
  }

  Atom * Molecule::CreateAtom()
  {
    qDebug() << "Molecule::CreateAtom()";
    Atom *atom = new Atom(this);
    connect(atom, SIGNAL(updated()), this, SLOT(updatePrimitive()));
    emit primitiveAdded(atom);
    return(atom);
  }
  
  Bond * Molecule::CreateBond()
  {
    qDebug() << "Molecule::CreateBond()";
    Bond *bond = new Bond(this);
    connect(bond, SIGNAL(updated()), this, SLOT(updatePrimitive()));
    emit primitiveAdded(bond);
    return(bond);
  }
  
  Residue * Molecule::CreateResidue()
  {
    Residue *residue = new Residue(this);
    connect(residue, SIGNAL(updated()), this, SLOT(updatePrimitive()));
    emit primitiveAdded(residue);
    return(residue);
  }
  
  void Molecule::DestroyAtom(OpenBabel::OBAtom *obatom)
  {
    qDebug() << "DestroyAtom Called";
    Atom *atom = static_cast<Atom *>(obatom);
    if(atom) {
      emit primitiveRemoved(atom);
      atom->deleteLater();
    }
  }
  
  void Molecule::DestroyBond(OpenBabel::OBBond *obbond)
  {
    qDebug() << "DestroyBond Called";
    Bond *bond = static_cast<Bond *>(obbond);
    if(bond) {
      emit primitiveRemoved(bond);
      bond->deleteLater();
    }
  }
  
  void Molecule::DestroyResidue(OpenBabel::OBResidue *obresidue)
  {
    qDebug() << "DestroyResidue Called";
    Residue *residue = static_cast<Residue *>(obresidue);
    if(residue) {
      emit primitiveRemoved(residue);
      residue->deleteLater();
    }
  }
  
  void Molecule::updatePrimitive()
  {
    Primitive *primitive = qobject_cast<Primitive *>(sender());
    emit primitiveUpdated(primitive);
    }
  
  void Molecule::centerAndFitInXYPlane()
  {
  Center();
  
    // count the atoms, check that there are any
    int numAtoms = 0;
    FOR_ATOMS_OF_MOL( a, this ) numAtoms++;
    if(!numAtoms) return;
    
    // compute the molecule's fitting plane
    int i = 0;
    Eigen::Vector3d * atomCenters = new Eigen::Vector3d[numAtoms];
    FOR_ATOMS_OF_MOL( a, this )
      atomCenters[i++] = Eigen::Vector3d( a->GetVector().AsArray() );
    Eigen::Vector4d planeCoeffs;
    Eigen::computeFittingHyperplane( numAtoms, atomCenters, &planeCoeffs );
    delete[] atomCenters;

    // compute rotation matrix to orient the molecule in the XY-plane
    Eigen::Vector3d planeNormalVector( & planeCoeffs(0) ), v, w;
    planeNormalVector.normalize();
    v.loadOrtho(planeNormalVector);
    w = cross( planeNormalVector, v );
    Eigen::Matrix3d rotation;
    rotation.setRow( 0, v );
    rotation.setRow( 1, w );
    rotation.setRow( 2, planeNormalVector );

    // apply rotation to each atom in the molecule
    FOR_ATOMS_OF_MOL( a, this )
    {
      Eigen::Vector3d atomCenter( a->GetVector().AsArray() );
      atomCenter = rotation * atomCenter;
      a->SetVector( atomCenter.x(),
                    atomCenter.y(),
                    atomCenter.z() );
    }

  }

  class PrimitiveQueuePrivate {
    public:
      PrimitiveQueuePrivate() {};
      
      QList< QList<Primitive *>* > queue;
  };

  PrimitiveQueue::PrimitiveQueue() : d(new PrimitiveQueuePrivate) { 
    for( int type=Primitive::FirstType; type<Primitive::LastType; type++ ) { 
      d->queue.append(new QList<Primitive *>()); 
    } 
  }

  PrimitiveQueue::~PrimitiveQueue() { 
    for( int i = 0; i<d->queue.size(); i++ ) { 
      delete d->queue[i];
    } 
    delete d;
  }

  const QList<Primitive *>* PrimitiveQueue::primitiveList(enum Primitive::Type type) const { 
    return(d->queue[type]); 
  }

  void PrimitiveQueue::addPrimitive(Primitive *p) { 
    d->queue[p->type()]->append(p); 
  }

  void PrimitiveQueue::removePrimitive(Primitive *p) {
    d->queue[p->type()]->removeAll(p);
  }

  int PrimitiveQueue::size() const {
    int sum = 0;
    for( int i=0; i<d->queue.size(); i++ ) {
      sum += d->queue[i]->size();
    }
    return sum;
  }

  void PrimitiveQueue::clear() {
    for( int i=0; i<d->queue.size(); i++ ) {
      d->queue[i]->clear();
    }
  }
}
