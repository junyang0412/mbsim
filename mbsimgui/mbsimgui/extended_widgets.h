/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012 Martin Förg

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

#ifndef _EXTENDED_WIDGETS_H_
#define _EXTENDED_WIDGETS_H_

#include "widget.h"
#include <QComboBox>
#include <QToolButton>
#include <QAction>
#include <QBoxLayout>

class QSpinBox;
class QStackedWidget;
class QListWidget;

namespace MBSimGUI {

  class ExtWidget : public Widget {
    Q_OBJECT

    public:
      ExtWidget(const QString &name, Widget *widget_, bool checkable=false, bool active=false, MBXMLUtils::FQN xmlName_="");
      Widget* getWidget() const { return widget; }
      void resize_(int m, int n) override { if(isActive()) widget->resize_(m,n); }
      bool isActive() const { return not toolButton->defaultAction()->isCheckable() or toolButton->defaultAction()->isChecked(); }
      void setActive(bool active) { if(toolButton->defaultAction()->isCheckable()) { toolButton->defaultAction()->setChecked(active); widget->setVisible(toolButton->defaultAction()->isChecked()); } }
      xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element) override;
      xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *parent, xercesc::DOMNode *ref=nullptr) override;
      void updateWidget() override { if(isActive()) widget->updateWidget(); }

    protected:
      Widget *widget;
      MBXMLUtils::FQN xmlName;
      QToolButton *toolButton;

    signals:
      void clicked(bool);
  };

  class ChoiceWidget : public Widget {
    Q_OBJECT

    public:
      ChoiceWidget(WidgetFactory *factory_, QBoxLayout::Direction dir=QBoxLayout::TopToBottom, int mode_=4);
      Widget* getWidget() const { return widget; }
      void updateWidget() override { widget->updateWidget(); }
      QString getName() const { return comboBox->currentText(); }
      int getIndex() const { return comboBox->currentIndex(); }
      void setIndex(int i) { return comboBox->setCurrentIndex(i); }
      void resize_(int m, int n) override { widget->resize_(m,n); }
      void setWidgetFactory(WidgetFactory *factory_);
      xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element) override;
      xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *parent, xercesc::DOMNode *ref=nullptr) override;
      void defineWidget(int);

    protected:
      QBoxLayout *layout;
      QComboBox *comboBox;
      Widget *widget;
      WidgetFactory *factory;
      int mode;

    signals:
      void comboChanged(int);
  };

  class ContainerWidget : public Widget {

    public:
      ContainerWidget();

      void resize_(int m, int n) override;
      void addWidget(Widget *widget_);
      Widget* getWidget(int i) const {return widget[i];}
      void updateWidget() override;
      xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element) override;
      xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *parent, xercesc::DOMNode *ref=nullptr) override;

    protected:
      QBoxLayout *layout;
      std::vector<Widget*> widget;
  };

  class ListWidget : public Widget {

    public:
      ListWidget(WidgetFactory *factory_, const QString &name_="Element", int m=0, int mode_=0, bool fixedSize=false, int minSize=0, int maxSize=100);
      ~ListWidget() override;
      void resize_(int m, int n) override;
      int getSize() const;
      void setSize(int m);
      Widget* getWidget(int i) const;
      xercesc::DOMElement* initializeUsingXML(xercesc::DOMElement *element) override;
      xercesc::DOMElement* writeXMLFile(xercesc::DOMNode *parent, xercesc::DOMNode *ref=nullptr) override;

    protected:
      void addElements(int n=1, bool emitSignals=true);
      void removeElements(int n=1);
      void changeCurrent(int idx);
      void currentIndexChanged(int idx);
      QStackedWidget *stackedWidget;
      QSpinBox* spinBox;
      QListWidget *list;
      WidgetFactory *factory;
      QString name;
      int mode;
  };

  class ChoiceWidgetFactory : public WidgetFactory {
    public:
      ChoiceWidgetFactory(WidgetFactory *factory_, int mode_=1) : factory(factory_), mode(mode_) { }
      Widget* createWidget(int i=0) override;
      MBXMLUtils::FQN getXMLName(int i=0) const override { return factory->getXMLName(i); }
    protected:
      WidgetFactory *factory;
      int mode;
  };

}

#endif
