/**********************************************************************
  BSDYEngine - Dynamic detail engine for "balls and sticks" display

  Copyright (C) 2007 Donald Ephraim Curtis

  This file is part of the Avogadro molecular editor project.
  For more information, see <http://avogadro.sourceforge.net/>

  Avogadro is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License as published by 
  the Free Software Foundation; either version 2 of the License, or 
  (at your option) any later version.

  Avogadro is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
 **********************************************************************/

#include <config.h>
#include "bsdyengine.h"

#include <avogadro/primitive.h>
#include <avogadro/color.h>
#include <avogadro/glwidget.h>
#include <avogadro/camera.h>

#include <openbabel/obiter.h>
#include <eigen/regression.h>

#include <QMessageBox>
#include <QSlider>
#include <QtPlugin>
#include <QVBoxLayout>

using namespace std;
using namespace OpenBabel;
using namespace Eigen;
using namespace Avogadro;

BSDYEngine::BSDYEngine(QObject *parent) : Engine(parent), m_glwidget(0),
  m_settingsWidget(0), m_atomRadiusPercentage(0.3), m_bondRadius(0.1)
{
  setName(tr("Dynamic Ball and Stick"));

  setDescription(tr("Renders primitives using Balls (atoms) and Sticks (bonds).  Includes demonstration of dynamic rendering based on distance from camera"));

}

BSDYEngine::~BSDYEngine()
{
  if(m_settingsWidget) {
    m_settingsWidget->deleteLater();
  }

}

bool BSDYEngine::render(GLWidget *gl)
{
  m_glwidget = gl;
  Color map = colorMap();

  QList<Primitive *> list;

  m_glwidget->painter()->begin(m_glwidget);

  glPushAttrib(GL_TRANSFORM_BIT);
  glDisable( GL_NORMALIZE );
  glEnable( GL_RESCALE_NORMAL );

  // Build up a list of the atoms and render them
  list = primitives().subList(Primitive::AtomType);
  foreach( Primitive *p, list )
  {
    Atom * a = static_cast<Atom *>(p);
    render(a);
  }

  // normalize normal vectors of bonds
  glDisable( GL_RESCALE_NORMAL);
  glEnable( GL_NORMALIZE );

  // Get a list of bonds and render them
  list = primitives().subList(Primitive::BondType);

  foreach( Primitive *p, list )
  {
    Bond *b = static_cast<Bond *>(p);
    render(b);
  }

  glPopAttrib();

  m_glwidget->painter()->end();
  return true;
}

bool BSDYEngine::render(const Atom* a)
{
  Color map = colorMap();

  // Push the atom type and name
  glPushName(Primitive::AtomType);
  glPushName(a->GetIdx());

  map.set(a);
  map.applyAsMaterials();

  m_glwidget->painter()->drawSphere( a->pos(), radius(a) );

  // Render the selection highlight
  if (m_glwidget->isSelected(a))
  {
    map.set( 0.3, 0.6, 1.0, 0.7 );
    map.applyAsMaterials();
    glEnable( GL_BLEND );
    m_glwidget->painter()->drawSphere( a->pos(), SEL_ATOM_EXTRA_RADIUS + radius(a) );
    glDisable( GL_BLEND );
  }

  glPopName();
  glPopName();

  return true;
}

bool BSDYEngine::render(const Bond* b)
{
  Color map = colorMap();

  // Push the type and name
  glPushName(Primitive::BondType);
  glPushName(b->GetIdx()+1);

  const Atom* atom1 = static_cast<const Atom *>(b->GetBeginAtom());
  const Atom* atom2 = static_cast<const Atom *>(b->GetEndAtom());
  Vector3d v1 (atom1->pos());
  Vector3d v2 (atom2->pos());
  Vector3d d = v2 - v1;
  d.normalize();
  Vector3d v3 ( (v1 + v2 + d*(radius(atom1)-radius(atom2))) / 2 );

  double shift = 0.15;
  int order = b->GetBO();

  map.set(atom1);
  map.applyAsMaterials();
  m_glwidget->painter()->drawMultiCylinder( v1, v3, m_bondRadius, order, shift );

  map.set(atom2);
  map.applyAsMaterials();
  m_glwidget->painter()->drawMultiCylinder( v3, v2, m_bondRadius, order, shift );

  // Render the selection highlight
  if (m_glwidget->isSelected(b))
  {
    map.set( 0.3, 0.6, 1.0, 0.7 );
    map.applyAsMaterials();
    glEnable( GL_BLEND );
    m_glwidget->painter()->drawMultiCylinder( v1, v2, SEL_BOND_EXTRA_RADIUS + m_bondRadius, order, shift );
    glDisable( GL_BLEND );
  }

  glPopName();
  glPopName();

  return true;
}

inline double BSDYEngine::radius(const Atom *atom)
{
  return etab.GetVdwRad(atom->GetAtomicNum()) * m_atomRadiusPercentage;
}

void BSDYEngine::setAtomRadiusPercentage(int percent)
{
  m_atomRadiusPercentage = 0.1 * percent;
  emit changed();
}

void BSDYEngine::setBondRadius(int value)
{
  m_bondRadius = value * 0.1;
  emit changed();
}

double BSDYEngine::radius(const Primitive *p)
{
  if (p->type() == Primitive::AtomType)
  {
    const Atom *a = static_cast<const Atom *>(p);
    double r = radius(a);
    if (m_glwidget)
    {
      if (m_glwidget->isSelected(p))
        return r + SEL_ATOM_EXTRA_RADIUS;
    }
    return r;
  }
  else
    return 0.;
}

bool BSDYEngine::render(const Molecule*)
{
  // Disabled
  return false;
}

QWidget *BSDYEngine::settingsWidget()
{
  if(!m_settingsWidget)
  {
    m_settingsWidget = new BSDYSettingsWidget();
    connect(m_settingsWidget->atomRadiusSlider, SIGNAL(valueChanged(int)), this, SLOT(setAtomRadiusPercentage(int)));
    connect(m_settingsWidget->bondRadiusSlider, SIGNAL(valueChanged(int)), this, SLOT(setBondRadius(int)));
    connect(m_settingsWidget, SIGNAL(destroyed()), this, SLOT(settingsWidgetDestroyed()));
  }
  return m_settingsWidget;
}

void BSDYEngine::settingsWidgetDestroyed()
{
  qDebug() << "Destroyed Settings Widget";
  m_settingsWidget = 0;
}

#include "bsdyengine.moc"

Q_EXPORT_PLUGIN2(bsdyengine, Avogadro::BSDYEngineFactory)
