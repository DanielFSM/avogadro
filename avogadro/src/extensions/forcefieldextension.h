/**********************************************************************
  forcefieldextension.h - Force field Plugin for Avogadro

  Copyright (C) 2006 by Donald Ephraim Curtis
  Copyright (C) 2006-2007 by Geoffrey R. Hutchison

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

#ifndef __FORCEFIELDEXTENSION_H
#define __FORCEFIELDEXTENSION_H


#include "forcefielddialog.h"
#include "constraintsmodel.h"
#include "constraintsdialog.h"

#include <openbabel/mol.h>
#include <openbabel/forcefield.h>

#include <avogadro/glwidget.h>
#include <avogadro/extension.h>

#include <QObject>
#include <QList>
#include <QString>
#include <QUndoCommand>
#include <QThread>
#include <QMutex>

#ifndef BUFF_SIZE
#define BUFF_SIZE 256
#endif

class QProgressDialog;
namespace Avogadro {

 class ForceFieldExtension : public QObject, public Extension
  {
    Q_OBJECT

    public:
      //! Constructor
      ForceFieldExtension(QObject *parent=0);
      //! Deconstructor
      virtual ~ForceFieldExtension();

      //! \name Description methods
      //@{
      //! Plugin Name (ie Draw)
      virtual QString name() const { return QObject::tr("ForceField"); }
      //! Plugin Description (ie. Draws atoms and bonds)
      virtual QString description() const { return QObject::tr("ForceField Plugin"); };

      /** @return a menu path for the extension's actions */
      virtual QString menuPath(QAction *action) const;

      //! Perform Action
      virtual QList<QAction *> actions() const;
      virtual QUndoCommand* performAction(QAction *action, Molecule *molecule,
                                          GLWidget *widget, QTextEdit *textEdit);
      //@}

    private:
      OpenBabel::OBForceField* m_forceField;
      ConstraintsModel* m_constraints;
      QList<QAction *> m_actions;
      ForceFieldDialog *m_Dialog;
      ConstraintsDialog *m_ConstraintsDialog;
  };

  class ForceFieldExtensionFactory : public QObject, public ExtensionFactory
  {
    Q_OBJECT;
    Q_INTERFACES(Avogadro::ExtensionFactory);

    public:
    Extension *createInstance(QObject *parent = 0) { return new ForceFieldExtension(parent); }
  };

  class ForceFieldThread : public QThread
  {
    Q_OBJECT;

    public:
    ForceFieldThread(Molecule *molecule, OpenBabel::OBForceField* forceField,
        ConstraintsModel* constraints, QTextEdit *textEdit,
	int forceFieldID, int nSteps, int algorithm, int gradients,
	int convergence, int task, QObject *parent=0);

      void run();
      int cycles() const;

    Q_SIGNALS:
      void stepsTaken(int steps);

    public Q_SLOTS:
      void stop();

    private:
      Molecule *m_molecule;
      ConstraintsModel* m_constraints;
      QTextEdit *m_textEdit;

      QMutex m_mutex;

      int m_cycles;
      int m_forceFieldID;
      int m_nSteps;
      int m_algorithm;
      int m_gradients;
      int m_convergence;
      int m_task;

      OpenBabel::OBForceField* m_forceField;
      ForceFieldDialog *m_Dialog;
      ConstraintsDialog *m_ConstraintsDialog;

      bool m_stop;
  };

 class ForceFieldCommand : public QUndoCommand
 {
   public:
     ForceFieldCommand(Molecule *molecule, OpenBabel::OBForceField *forcefield,
         ConstraintsModel* constraints, QTextEdit *messages, int forceFieldID,
	 int nSteps, int algorithm, int gradients, int convergence, int task);

     ~ForceFieldCommand();

     virtual void redo();
     virtual void undo();
     virtual bool mergeWith ( const QUndoCommand * command );
     virtual int id() const;

     void detach() const;
     void cleanup();

     ForceFieldThread *thread() const;
     QProgressDialog *progressDialog() const;

   private:
     Molecule m_moleculeCopy;

     int m_nSteps;
     int m_task;
     Molecule *m_molecule;
     ConstraintsModel* m_constraints;
     QTextEdit *m_textEdit;

     ForceFieldThread *m_thread;
     QProgressDialog *m_dialog;

     mutable bool m_detached;

 };

} // end namespace Avogadro

#endif
