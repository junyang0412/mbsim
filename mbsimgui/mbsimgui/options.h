/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012-2014 Martin Förg

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __DIALOG_H_
#define __DIALOG_H_

#include <QDialog>

class QCheckBox;
class QSpinBox;
class QLineEdit;
class QPushButton;

namespace MBSimGUI {

  class OptionsDialog : public QDialog {
    Q_OBJECT

    public:
      OptionsDialog(QWidget *parent);
      bool getSaveStateVector() const;
      void setSaveStateVector(bool flag);
      bool getAutoSave() const;
      void setAutoSaveInterval(int min);
      int getAutoSaveInterval() const;
      void setAutoSave(bool flag);
      bool getAutoExport() const;
      void setAutoExport(bool flag);
      QString getAutoExportDir() const;
      void setAutoExportDir(const QString &dir);
      void setMaxUndo(int num);
      int getMaxUndo() const;
      bool getShowFilters() const;
      void setShowFilters(bool flag);
      bool getAutoRefresh() const;
      void setAutoRefresh(bool flag);
    private:
      QCheckBox *autoSave, *autoExport, *saveStateVector, *showFilters, *autoRefresh;
      QSpinBox *autoSaveInterval, *maxUndo;
      QLineEdit *autoExportDir;
      QPushButton *button;
    protected slots:
      void autoSaveChanged(int state);
      void autoExportChanged(int state);
      void openFileBrowser();
  };

}

#endif
