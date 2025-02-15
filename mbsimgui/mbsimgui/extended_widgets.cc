/*
    MBSimGUI - A fronted for MBSim.
    Copyright (C) 2012 Martin Förg

  This library is free software; you can redistribute it and/or 
  modify it under the terms of the GNU Lesser General Public 
  License as published by the Free Software Foundation; either 
  version 2.1 of the License, or (at your option) any later version. 
   
  This library is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
  Lesser General Public License for more details. 
   
  You should have received a copy of the GNU Lesser General Public 
  License along with this library; if not, write to the Free Software 
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
*/

#include <config.h>
#include <cassert>
#include "extended_widgets.h"
#include "variable_widgets.h"
#include "dialogs.h"
#include "custom_widgets.h"
#include "unknown_widget.h"
#include <QListWidget>
#include <QStackedWidget>
#include <utility>

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;

namespace MBSimGUI {

  bool MouseEvent::eventFilter(QObject *obj, QEvent *event) {
    if(event->type() == QEvent::MouseButtonPress) {
      emit mousePressed();
      return true;
    }
    else
      return QObject::eventFilter(obj, event);
  }

  ExtWidget::ExtWidget(const QString &name, Widget *widget_, bool checkable_, bool active, FQN xmlName_) : checkable(checkable_), checked(active), widget(widget_), xmlName(std::move(xmlName_)) {

    auto *layout = new QVBoxLayout;
    layout->setMargin(0);
    setLayout(layout);

    label = new QLabel;
    layout->addWidget(label);
    layout->addWidget(widget);
    widget->setContentsMargins(10,0,0,0);
    MouseEvent *mouseEvent = new MouseEvent(label);
    label->installEventFilter(mouseEvent);
    checkable = checkable_;
    checked = active;
    if(checkable) {
      label->setEnabled(active);
      label->setText(name + " <small>optional</small>");
      label->setToolTip("Click to define or remove this property");
      widget->setVisible(active);
      connect(mouseEvent,&MouseEvent::mousePressed,this,[=]{
          setActive(not checked);
          emit widgetChanged();
          emit clicked(checked);
          });
    }
    else {
      label->setToolTip("This property must be defined");
      label->setText(name);
    }
    connect(widget,&Widget::widgetChanged,this,&ExtWidget::widgetChanged);
  }

  void ExtWidget::setActive(bool active) {
    if(checkable) {
      checked = active;
      label->setEnabled(checked);
      widget->setVisible(checked);
    }
  }

  DOMElement* ExtWidget::initializeUsingXML(DOMElement *element) {
    bool active = false;
    if(xmlName!=FQN()) {
      DOMElement *e=E(element)->getFirstElementChildNamed(xmlName);
      if(e)
        active = widget->initializeUsingXML(e);
    }
    else
      active = widget->initializeUsingXML(element);
    setActive(active);
    return active?element:nullptr;
  }

  DOMElement* ExtWidget::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    DOMElement *ele = nullptr;
    if(xmlName!=FQN()) {
      DOMDocument *doc = parent->getOwnerDocument();
      DOMElement *newele = D(doc)->createElement(xmlName);
      if(isActive()) {
        ele = widget->writeXMLFile(newele);
        parent->insertBefore(newele,ref);
      }
    }
    else {
      if(isActive())
        ele = widget->writeXMLFile(parent,ref);
    }
    return ele;
  }

  ChoiceWidget::ChoiceWidget(WidgetFactory *factory_, QBoxLayout::Direction dir, int mode_) : widget(nullptr), factory(factory_), mode(mode_) {
    layout = new QBoxLayout(dir);
    layout->setMargin(0);
    setLayout(layout);

    comboBox = new CustomComboBox;
    for(int i=0; i<factory->getSize(); i++)
      comboBox->addItem(factory->getName(i));
    layout->addWidget(comboBox);
    comboBox->setCurrentIndex(factory->getDefaultIndex());
    defineWidget(factory->getDefaultIndex());
    connect(comboBox,QOverload<int>::of(&CustomComboBox::currentIndexChanged),this,&ChoiceWidget::defineWidget);
    connect(comboBox,QOverload<int>::of(&CustomComboBox::currentIndexChanged),this,&ChoiceWidget::comboChanged);
  }

  void ChoiceWidget::setWidgetFactory(WidgetFactory *factory_) {
    factory = factory_;
    comboBox->blockSignals(true);
    comboBox->clear();
    for(int i=0; i<factory->getSize(); i++)
      comboBox->addItem(factory->getName(i));
    comboBox->setCurrentIndex(factory->getDefaultIndex());
    defineWidget(factory->getDefaultIndex());
    comboBox->blockSignals(false);
  }

  void ChoiceWidget::defineWidget(int index) {
    if(widget) {
      layout->removeWidget(widget);
      delete widget;
    }
    assert(index!=-1 && "index==-1 is no longer supported!");
    widget = factory->createWidget(index);
    if(layout->direction()==QBoxLayout::TopToBottom)
      widget->setContentsMargins(factory->getMargin(),0,0,0);
    layout->addWidget(widget);
    emit widgetChanged();
    connect(widget,&Widget::widgetChanged,this,&ChoiceWidget::widgetChanged);
  }

  DOMElement* ChoiceWidget::initializeUsingXML(DOMElement *element) {
    if (mode<=1) {
      DOMElement *e=(mode==0)?element->getFirstElementChild():element;
      if(e) {
        int k = factory->getFallbackIndex();
        for(int i=0; i<factory->getSize(); i++) {
          if(E(e)->getTagName()==factory->getXMLName(i)) {
            k = i;
            break;
          }
        }
        blockSignals(true);
        defineWidget(k);
        blockSignals(false);
        comboBox->blockSignals(true);
        comboBox->setCurrentIndex(k);
        comboBox->blockSignals(false);
        return widget->initializeUsingXML(e);
      }
    }
    else if (mode<=3) {
      for(int i=0; i<factory->getSize(); i++) {
        DOMElement *e=E(element)->getFirstElementChildNamed(factory->getXMLName(i));
        if(e) {
          blockSignals(true);
          defineWidget(i);
          blockSignals(false);
          comboBox->blockSignals(true);
          comboBox->setCurrentIndex(i);
          comboBox->blockSignals(false);
          return widget->initializeUsingXML(e);
        }
      }
    }
    else {
      for(int i=0; i<factory->getSize(); i++) {
        blockSignals(true);
        defineWidget(i);
        blockSignals(false);
        comboBox->blockSignals(true);
        comboBox->setCurrentIndex(i);
        comboBox->blockSignals(false);
        if(widget->initializeUsingXML(element))
          return element;
      }
    }
    return nullptr;
  }

  DOMElement* ChoiceWidget::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    if(mode==3) {
      xercesc::DOMDocument *doc=parent->getOwnerDocument();
      DOMElement *ele0 = D(doc)->createElement(factory->getXMLName(getIndex()));
      widget->writeXMLFile(ele0);
      parent->insertBefore(ele0,ref);
    }
    else
      widget->writeXMLFile(parent,ref);
    return nullptr;
  }

  ContainerWidget::ContainerWidget() {
    layout = new QVBoxLayout;
    setLayout(layout);
    layout->setMargin(0);
  }

  void ContainerWidget::resize_(int m, int n) {
    for(auto & i : widget)
      i->resize_(m,n);
  }

  void ContainerWidget::addWidget(Widget *widget_) {
    layout->addWidget(widget_); 
    widget.push_back(widget_);
    connect(widget[widget.size()-1],&Widget::widgetChanged,this,&ChoiceWidget::widgetChanged);
  }

  void ContainerWidget::updateWidget() {
    for(unsigned int i=0; i<widget.size(); i++)
      getWidget(i)->updateWidget();
  }

  DOMElement* ContainerWidget::initializeUsingXML(DOMElement *element) {
    bool flag = false;
    for(unsigned int i=0; i<widget.size(); i++)
      if(getWidget(i)->initializeUsingXML(element))
        flag = true;
    return flag?element:nullptr;
  }

  DOMElement* ContainerWidget::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    for(unsigned int i=0; i<widget.size(); i++)
      getWidget(i)->writeXMLFile(parent,ref);
    return nullptr;
  }

  ListWidget::ListWidget(WidgetFactory *factory_, const QString &name_, int m, int mode_, bool fixedSize, int minSize, int maxSize) : factory(factory_), name(name_), mode(mode_) {
    auto *layout = new QGridLayout;
    layout->setMargin(0);
    setLayout(layout);

    spinBox = new CustomSpinBox;
    spinBox->setDisabled(fixedSize);
    spinBox->setRange(minSize,maxSize);
    spinBox->setValue(m);
    layout->addWidget(new QLabel("Number:"),0,0);
    layout->addWidget(spinBox,0,1);
    connect(spinBox,QOverload<int>::of(&CustomSpinBox::valueChanged),this,&ListWidget::currentIndexChanged);
    list = new QListWidget;
    layout->addWidget(list,1,0,1,3);
    stackedWidget = new QStackedWidget;
    connect(list,&QListWidget::currentRowChanged,this,&ListWidget::changeCurrent);
    layout->addWidget(stackedWidget,2,0,1,3);
    layout->setColumnStretch(2,1);
    currentIndexChanged(m);
  }

  ListWidget::~ListWidget() {
    delete factory;
  }

  int ListWidget::getSize() const {
    return stackedWidget->count();
  }

  void ListWidget::setSize(int m) {
    spinBox->setValue(m);
  }

  Widget* ListWidget::getWidget(int i) const {
    return static_cast<Widget*>(stackedWidget->widget(i));
  }

  void ListWidget::currentIndexChanged(int idx) {
    int n = idx - list->count();
    if(n>0)
      addElements(n);
    else if(n<0)
      removeElements(-n);
  }

  void ListWidget::changeCurrent(int idx) {
    if(idx>=0) {
      if (stackedWidget->currentWidget() !=nullptr)
        stackedWidget->currentWidget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
      stackedWidget->setCurrentIndex(idx);
      stackedWidget->currentWidget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      adjustSize();
    }
  }

  void ListWidget::resize_(int m, int n) {
    for(int i=0; i<stackedWidget->count(); i++)
      getWidget(i)->resize_(m,n);
  }

  void ListWidget::addElements(int n, bool emitSignals) {

    int i = stackedWidget->count();

    for(int j=1; j<=n; j++) {
      list->addItem(name+" "+QString::number(i+j));

      Widget *widget = factory->createWidget();
      stackedWidget->addWidget(widget);
      if(i>0)
        widget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

      connect(widget,&Widget::widgetChanged,this,&ListWidget::widgetChanged);
    }

    if(i==0)
      list->setCurrentRow(0);

    if(emitSignals)
      emit Widget::widgetChanged();
  }

  void ListWidget::removeElements(int n) {
    for(int j=0; j<n; j++) {
      int i = list->count()-1;
      delete stackedWidget->widget(i);
      stackedWidget->removeWidget(stackedWidget->widget(i));
      delete list->takeItem(i);
    }
    emit Widget::widgetChanged();
  }

  DOMElement* ListWidget::initializeUsingXML(DOMElement *element) {
    blockSignals(true);
    removeElements(getSize());
    blockSignals(false);
    list->blockSignals(true);
    spinBox->blockSignals(true);
    if(mode<=1) {
      DOMElement *e=(mode==0)?element->getFirstElementChild():element;
      while(e) {
        addElements(1,false);
        getWidget(getSize()-1)->initializeUsingXML(e);
        e=e->getNextElementSibling();
      }
      spinBox->setValue(getSize());
    }
    else if(mode==2) {
      DOMElement *e=E(element)->getFirstElementChildNamed(factory->getXMLName());
      while(e and E(e)->getTagName()==factory->getXMLName()) {
        addElements(1,false);
        getWidget(getSize()-1)->initializeUsingXML(e);
        e=e->getNextElementSibling();
      }
      spinBox->setValue(getSize());
    }
    else {
      DOMElement *e=E(element)->getFirstElementChildNamed(factory->getXMLName());
      while(e and E(e)->getTagName()==factory->getXMLName()) {
        addElements(1,false);
        e = getWidget(getSize()-1)->initializeUsingXML(e);
      }
      spinBox->setValue(getSize());
    }
    list->blockSignals(false);
    spinBox->blockSignals(false);
    return element;
  }

  DOMElement* ListWidget::writeXMLFile(DOMNode *parent, DOMNode *ref) {
    if(mode<=1 or mode==3) {
      for(unsigned int i=0; i<getSize(); i++)
        getWidget(i)->writeXMLFile(parent,ref);
    }
    else {
      xercesc::DOMDocument *doc=parent->getOwnerDocument();
      for(unsigned int i=0; i<getSize(); i++) {
        DOMElement *ele0 = D(doc)->createElement(factory->getXMLName());
        getWidget(i)->writeXMLFile(ele0);
        parent->insertBefore(ele0,ref);
      }
    }
    return nullptr;
  }

  Widget* ChoiceWidgetFactory::createWidget(int i) {
    return new ChoiceWidget(factory,QBoxLayout::TopToBottom,mode);
  }

}
