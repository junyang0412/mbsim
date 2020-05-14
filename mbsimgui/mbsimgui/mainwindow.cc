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

#include <config.h>
#include "mainwindow.h"
#include "options.h"
#include "dynamic_system_solver.h"
#include "frame.h"
#include "contour.h"
#include "object.h"
#include "link_.h"
#include "constraint.h"
#include "observer.h"
#include "integrator.h"
#include "analyzer.h"
#include "objectfactory.h"
#include "parameter.h"
#include "widget.h"
#include "treemodel.h"
#include "treeitem.h"
#include "element_view.h"
#include "parameter_view.h"
#include "solver_view.h"
#include "project_view.h"
#include "echo_view.h"
#include "embed.h"
#include "project.h"
#include "project_property_dialog.h"
#include "file_editor.h"
#include "utils.h"
#include "basicitemdata.h"
#include <openmbv/mainwindow.h>
#include <utime.h>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QSettings>
#include <QHeaderView>
#include <QtWidgets/QDesktopWidget>
#include <QDesktopServices>
#include <mbxmlutils/eval.h>
#include <mbxmlutils/preprocess.h>
#include <boost/dll.hpp>
#include <mbxmlutilshelper/dom.h>
#include <xercesc/dom/DOMProcessingInstruction.hpp>
#include <xercesc/dom/DOMException.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMLSSerializer.hpp>
#include "dialogs.h"

using namespace std;
using namespace MBXMLUtils;
using namespace xercesc;
namespace bfs=boost::filesystem;

namespace MBSimGUI {

  bool currentTask;

  MainWindow *mw;

  vector<boost::filesystem::path> dependencies;

  MainWindow::MainWindow(QStringList &arg) : project(nullptr), inlineOpenMBVMW(nullptr), allowUndo(true), maxUndo(10), autoRefresh(true), doc(nullptr), elementBuffer(NULL,false), parameterBuffer(NULL,false), installPath(boost::dll::program_location().parent_path().parent_path()) {
    QSettings settings;

    impl=DOMImplementation::getImplementation();
    parser=impl->createLSParser(DOMImplementation::MODE_SYNCHRONOUS, nullptr);
    serializer=impl->createLSSerializer();
    basicSerializer=impl->createLSSerializer();

    // use html output of MBXMLUtils
    static string HTMLOUTPUT="MBXMLUTILS_ERROROUTPUT=HTMLXPATH";
    putenv(const_cast<char*>(HTMLOUTPUT.c_str()));

    serializer->getDomConfig()->setParameter(X()%"format-pretty-print", true);

    mw = this;

#if _WIN32
    uniqueTempDir=bfs::unique_path(bfs::temp_directory_path()/"mbsimgui_%%%%-%%%%-%%%%-%%%%");
#else
    if(bfs::is_directory("/dev/shm"))
      uniqueTempDir=bfs::unique_path("/dev/shm/mbsimgui_%%%%-%%%%-%%%%-%%%%");
    else
      uniqueTempDir=bfs::unique_path(bfs::temp_directory_path()/"mbsimgui_%%%%-%%%%-%%%%-%%%%");
#endif
    bfs::create_directories(uniqueTempDir);

    QString program = QString::fromStdString((boost::dll::program_location().parent_path().parent_path()/"bin"/"mbsimxml").string());
    QStringList arguments;
    arguments << "--onlyListSchemas";
    QProcess processGetSchemas;
    processGetSchemas.start(program,arguments);
    processGetSchemas.waitForFinished(-1);
    QStringList line=QString(processGetSchemas.readAllStandardOutput().data()).split("\n");
    set<bfs::path> schemas;
    for(int i=0; i<line.size(); ++i)
      if(!line.at(i).isEmpty())
        schemas.insert(line.at(i).toStdString());

    mbxmlparser=DOMParser::create(schemas);

    projectView = new ProjectView;
    elementView = new ElementView;
    parameterView = new ParameterView;
    solverView = new SolverView;
    echoView = new EchoView(this);

    // initialize streams
    auto f=[this](const string &s){
      echoView->addOutputText(QString::fromStdString(s));
    };
    debugStreamFlag=std::make_shared<bool>(false);
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Info      , std::make_shared<bool>(true),
      make_shared<fmatvec::PrePostfixedStream>("<span class=\"MBSIMGUI_INFO\">", "</span>", f));
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Warn      , std::make_shared<bool>(true),
      make_shared<fmatvec::PrePostfixedStream>("<span class=\"MBSIMGUI_WARN\">", "</span>", f));
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Debug     , debugStreamFlag             ,
      make_shared<fmatvec::PrePostfixedStream>("<span class=\"MBSIMGUI_DEBUG\">", "</span>", f));
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Error     , std::make_shared<bool>(true),
      make_shared<fmatvec::PrePostfixedStream>("<span class=\"MBSIMGUI_ERROR\">", "</span>", f));
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Deprecated, std::make_shared<bool>(true),
      make_shared<fmatvec::PrePostfixedStream>("<span class=\"MBSIMGUI_DEPRECATED\">", "</span>", f));
    fmatvec::Atom::setCurrentMessageStream(fmatvec::Atom::Status    , std::make_shared<bool>(true),
      make_shared<fmatvec::PrePostfixedStream>("", "", [this](const string &s){
        // call this function only every 0.25 sec
        if(getStatusTime().elapsed()<250) return;
        getStatusTime().restart();
        // print to status bar
        statusBar()->showMessage(QString::fromStdString(s));
      }));

    initInlineOpenMBV();

    MBSimObjectFactory::initialize();

    QMenu *GUIMenu = new QMenu("GUI", menuBar());
    menuBar()->addMenu(GUIMenu);

    QAction *action = GUIMenu->addAction(QIcon::fromTheme("document-properties"), "Options", this, &MainWindow::openOptionsMenu);
    action->setStatusTip(tr("Open options menu"));

    GUIMenu->addSeparator();

    elementViewFilter = new OpenMBVGUI::AbstractViewFilter(elementView, 0, 1);
    elementViewFilter->hide();

    parameterViewFilter = new OpenMBVGUI::AbstractViewFilter(parameterView, 0, -2);
    parameterViewFilter->hide();

    GUIMenu->addSeparator();

    action = GUIMenu->addAction(QIcon::fromTheme("application-exit"), "E&xit", this, &MainWindow::close);
    action->setShortcut(QKeySequence::Quit);
    action->setStatusTip(tr("Exit the application"));

    for (auto & recentProjectFileAct : recentProjectFileActs) {
      recentProjectFileAct = new QAction(this);
      recentProjectFileAct->setVisible(false);
      connect(recentProjectFileAct, &QAction::triggered, this, &MainWindow::openRecentProjectFile);
    }
    QMenu *menu = new QMenu("Project", menuBar());
    action = menu->addAction(QIcon::fromTheme("document-new"), "New", this, &MainWindow::newProject);
    action->setShortcut(QKeySequence::New);
    action = menu->addAction(QIcon::fromTheme("document-open"), "Open", this, QOverload<>::of(&MainWindow::loadProject));
    action->setShortcut(QKeySequence::Open);
    action = menu->addAction(QIcon::fromTheme("document-save-as"), "Save as", this, &MainWindow::saveProjectAs);
    action->setShortcut(QKeySequence::SaveAs);
    actionSaveProject = menu->addAction(QIcon::fromTheme("document-save"), "Save", this, [=](){ this->saveProject(); });
    actionSaveProject->setShortcut(QKeySequence::Save);
    menu->addSeparator();
    for (auto & recentProjectFileAct : recentProjectFileActs)
      menu->addAction(recentProjectFileAct);
    updateRecentProjectFileActions();
    menuBar()->addMenu(menu);

    menu = new QMenu("Edit", menuBar());
    action = menu->addAction(QIcon::fromTheme("document-properties"), "Edit", this, &MainWindow::edit);
    action->setShortcut(QKeySequence("Ctrl+E"));
    menu->addSeparator();
    actionUndo = menu->addAction(QIcon::fromTheme("edit-undo"), "Undo", this, &MainWindow::undo);
    actionUndo->setShortcut(QKeySequence::Undo);
    actionUndo->setDisabled(true);
    actionRedo = menu->addAction(QIcon::fromTheme("edit-redo"), "Redo", this, &MainWindow::redo);
    actionRedo->setShortcut(QKeySequence::Redo);
    actionRedo->setDisabled(true);
    menu->addSeparator();
    action = menu->addAction(QIcon::fromTheme("edit-copy"), "Copy", this, &MainWindow::copy);
    action->setShortcut(QKeySequence::Copy);
    action = menu->addAction(QIcon::fromTheme("edit-cut"), "Cut", this, &MainWindow::cut);
    action->setShortcut(QKeySequence::Cut);
    action = menu->addAction(QIcon::fromTheme("edit-paste"), "Paste", this, &MainWindow::paste);
    action->setShortcut(QKeySequence::Paste);
    menu->addSeparator();
    action = menu->addAction(QIcon::fromTheme("edit-delete"), "Remove", this, &MainWindow::remove);
    action->setShortcut(QKeySequence::Delete);
    menu->addSeparator();
    action = menu->addAction(QIcon::fromTheme("go-up"), "Move up", this, &MainWindow::moveUp);
    action->setShortcut(QKeySequence("Ctrl+Up"));
    action = menu->addAction(QIcon::fromTheme("go-down"), "Move down", this, &MainWindow::moveDown);
    action->setShortcut(QKeySequence("Ctrl+Down"));
    menuBar()->addMenu(menu);

    menu = new QMenu("Export", menuBar());
    actionSaveDataAs = menu->addAction("Export all data", this, &MainWindow::saveDataAs);
    actionSaveMBSimH5DataAs = menu->addAction("Export MBSim data file", this, &MainWindow::saveMBSimH5DataAs);
    actionSaveOpenMBVDataAs = menu->addAction("Export OpenMBV data", this, &MainWindow::saveOpenMBVDataAs);
    actionSaveStateVectorAs = menu->addAction("Export state vector", this, &MainWindow::saveStateVectorAs);
    actionSaveEigenanalysisAs = menu->addAction("Export eigenanalysis", this, &MainWindow::saveEigenanalysisAs);
    menuBar()->addMenu(menu);

    menuBar()->addSeparator();
    QMenu *helpMenu = new QMenu("Help", menuBar());
    helpMenu->addAction(QIcon::fromTheme("help-contents"), "Contents", this, &MainWindow::help);
    helpMenu->addAction(QIcon::fromTheme("help-xml"), "XML Help", this, [=](){ this->xmlHelp(); });
    helpMenu->addAction(QIcon::fromTheme("help-about"), "About", this, &MainWindow::about);
    menuBar()->addMenu(helpMenu);

    QToolBar *toolBar = addToolBar("Tasks");
    toolBar->setObjectName("toolbar/tasks");
    actionSimulate = toolBar->addAction(style()->standardIcon(QStyle::StandardPixmap(QStyle::SP_MediaPlay)),"Start simulation");
    actionSimulate->setStatusTip(tr("Simulate the multibody system"));
    connect(actionSimulate,&QAction::triggered,this,&MainWindow::simulate);
    toolBar->addAction(actionSimulate);
    QAction *actionInterrupt = toolBar->addAction(style()->standardIcon(QStyle::StandardPixmap(QStyle::SP_MediaStop)),"Interrupt simulation");
    connect(actionInterrupt,&QAction::triggered,this,&MainWindow::interrupt);
    toolBar->addAction(actionInterrupt);
    actionRefresh = toolBar->addAction(style()->standardIcon(QStyle::StandardPixmap(QStyle::SP_BrowserReload)),"Refresh 3D view");
    connect(actionRefresh,&QAction::triggered,this,&MainWindow::refresh);
    toolBar->addAction(actionRefresh);
    actionOpenMBV = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"openmbv.svg").string())),"OpenMBV");
    connect(actionOpenMBV,&QAction::triggered,this,&MainWindow::openmbv);
    toolBar->addAction(actionOpenMBV);
    actionH5plotserie = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"h5plotserie.svg").string())),"H5plotserie");
    connect(actionH5plotserie,&QAction::triggered,this,&MainWindow::h5plotserie);
    toolBar->addAction(actionH5plotserie);
    actionEigenanalysis = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"eigenanalysis.svg").string())),"Eigenanalysis");
    connect(actionEigenanalysis,&QAction::triggered,this,&MainWindow::eigenanalysis);
    toolBar->addAction(actionEigenanalysis);
    actionFrequencyResponse = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"frequency_response.svg").string())),"Harmonic response analysis");
    connect(actionFrequencyResponse,&QAction::triggered,this,&MainWindow::frequencyResponse);
    actionDebug = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"debug.svg").string())),"Debug model");
    connect(actionDebug,&QAction::triggered,this,&MainWindow::debug);
    toolBar->addAction(actionDebug);
    QAction *actionKill = toolBar->addAction(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"kill.svg").string())),"Kill simulation");
    connect(actionKill,&QAction::triggered,this,&MainWindow::kill);
    toolBar->addAction(actionKill);

    elementView->setModel(new ElementTreeModel(this));
    //elementView->setColumnWidth(0,250);
    //elementView->setColumnWidth(1,200);
    elementView->hideColumn(1);

    parameterView->setModel(new ParameterTreeModel(this));
    //parameterView->setColumnWidth(0,150);
    //parameterView->setColumnWidth(1,200);

    connect(elementView, &ElementView::pressed, this, &MainWindow::elementViewClicked);
    connect(parameterView, &ParameterView::pressed, this, &MainWindow::parameterViewClicked);
    connect(elementView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::selectionChanged);

    QDockWidget *dockWidget0 = new QDockWidget("MBSim project", this);
    dockWidget0->setObjectName("dockWidget/project");
    addDockWidget(Qt::LeftDockWidgetArea,dockWidget0);
    dockWidget0->setWidget(projectView);

    QDockWidget *dockWidget1 = new QDockWidget("Multibody system", this);
    dockWidget1->setObjectName("dockWidget/mbs");
    addDockWidget(Qt::LeftDockWidgetArea,dockWidget1);
    QWidget *widget1 = new QWidget(dockWidget1);
    dockWidget1->setWidget(widget1);
    auto *widgetLayout1 = new QGridLayout(widget1);
    widgetLayout1->setContentsMargins(0,0,0,0);
    widget1->setLayout(widgetLayout1);
    widgetLayout1->addWidget(elementViewFilter, 0, 0);
    widgetLayout1->addWidget(elementView, 1, 0);

    QDockWidget *dockWidget3 = new QDockWidget("Parameters", this);
    dockWidget3->setObjectName("dockWidget/parameters");
    addDockWidget(Qt::LeftDockWidgetArea,dockWidget3);
    QWidget *widget3 = new QWidget(dockWidget3);
    dockWidget3->setWidget(widget3);
    auto *widgetLayout3 = new QGridLayout(widget3);
    widgetLayout3->setContentsMargins(0,0,0,0);
    widget3->setLayout(widgetLayout3);
    widgetLayout3->addWidget(parameterViewFilter, 0, 0);
    widgetLayout3->addWidget(parameterView, 1, 0);

    QDockWidget *dockWidget2 = new QDockWidget("Solver", this);
    dockWidget2->setObjectName("dockWidget/solver");
    addDockWidget(Qt::LeftDockWidgetArea,dockWidget2);
    dockWidget2->setWidget(solverView);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto *mainlayout = new QHBoxLayout;
    centralWidget->setLayout(mainlayout);
    mainlayout->addWidget(inlineOpenMBVMW);

    QDockWidget *mbsimDW = new QDockWidget("MBSim Echo Area", this);
    mbsimDW->setObjectName("dockWidget/echoArea");
    addDockWidget(Qt::BottomDockWidgetArea, mbsimDW);
    mbsimDW->setWidget(echoView);
    connect(&process,QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),this,&MainWindow::processFinished);
    connect(&process,&QProcess::readyReadStandardOutput,this,&MainWindow::updateEchoView);
    connect(&process,&QProcess::readyReadStandardError,this,&MainWindow::updateStatus);

    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

    if(arg.contains("--maximized"))
      showMaximized();

    QString projectFile;
    QRegExp filterProject(".+\\.mbsx");
    QDir dir;
    dir.setFilter(QDir::Files);
    for(auto & it : arg) {
      if(it[0]=='-') continue;
      dir.setPath(it);
      if(dir.exists()) {
        QStringList file=dir.entryList();
        for(int j=0; j<file.size(); j++) {
          if(projectFile.isEmpty() and filterProject.exactMatch(file[j]))
            projectFile = dir.path()+"/"+file[j];
        }
        continue;
      }
      if(QFile::exists(it)) {
        if(projectFile.isEmpty())
          projectFile = it;
        continue;
      }
    }
    if(projectFile.size())
      loadProject(QDir::current().absoluteFilePath(projectFile));
    else
      newProject();

    setAcceptDrops(true);

    connect(&autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSaveProject);
    autoSaveTimer.start(settings.value("mainwindow/options/autosaveinterval", 5).toInt()*60000);
    statusTime.start();

    setWindowIcon(Utils::QIconCached(QString::fromStdString((installPath/"share"/"mbsimgui"/"icons"/"mbsimgui.svg").string())));

    // auto exit if everything is finished
    if(arg.contains("--autoExit")) {
      auto timer=new QTimer(this);
      connect(timer, &QTimer::timeout, [this, timer](){
        if(process.state()==QProcess::NotRunning) {
          timer->stop();
          if(!close())
            timer->start(100);
        }
      });
      timer->start(100);
    }

    openOptionsMenu(true);
  }

  void MainWindow::autoSaveProject() {
    saveProject("./.Project.mbsx",false);
  }

  void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    updateEchoView();
    if(currentTask==1 && bfs::exists(uniqueTempDir.generic_string()+"/MBS_tmp.ombvx") && process.state()==QProcess::NotRunning) {
      inlineOpenMBVMW->openFile(uniqueTempDir.generic_string()+"/MBS_tmp.ombvx");
      QModelIndex index = elementView->selectionModel()->currentIndex();
      auto *model = static_cast<ElementTreeModel*>(elementView->model());
      auto *element=dynamic_cast<Element*>(model->getItem(index)->getItemData());
      if(element)
        highlightObject(element->getID());
    }
    else {
      if(exitStatus == QProcess::NormalExit) {
        QSettings settings;
        bool saveFinalStateVector = settings.value("mainwindow/options/savestatevector", false).toBool();
        if(settings.value("mainwindow/options/autoexport", false).toBool()) {
          QString autoExportDir = settings.value("mainwindow/options/autoexportdir", "./").toString();
          saveMBSimH5Data(autoExportDir+"/MBS.mbsh5");
          saveOpenMBVXMLData(autoExportDir+"/MBS.ombvx");
          saveOpenMBVH5Data(autoExportDir+"/MBS.ombvh5");
          if(saveFinalStateVector)
            saveStateVector(autoExportDir+"/statevector.asc");
        }
        actionSaveDataAs->setDisabled(false);
        actionSaveMBSimH5DataAs->setDisabled(false);
        actionSaveOpenMBVDataAs->setDisabled(false);
        if(saveFinalStateVector)
          actionSaveStateVectorAs->setDisabled(false);
        if(dynamic_cast<Eigenanalyzer*>(getProject()->getSolver())) {
          actionSaveEigenanalysisAs->setDisabled(false);
          actionEigenanalysis->setDisabled(false);
        }
        if(dynamic_cast<HarmonicResponseAnalyzer*>(getProject()->getSolver())) {
          actionFrequencyResponse->setDisabled(false);
        }
        actionOpenMBV->setDisabled(false);
        actionH5plotserie->setDisabled(false);
      }
      else {
      }
    }
    actionSimulate->setDisabled(false);
    actionRefresh->setDisabled(false);
    actionDebug->setDisabled(false);
    statusBar()->showMessage(tr("Ready"));
  }

  void MainWindow::initInlineOpenMBV() {
    std::list<string> arg;
    arg.emplace_back("--wst");
    arg.push_back((installPath/"share"/"mbsimgui"/"inlineopenmbv.ombvwst").string());
    inlineOpenMBVMW = new OpenMBVGUI::MainWindow(arg);

    connect(inlineOpenMBVMW, &OpenMBVGUI::MainWindow::objectSelected, this, &MainWindow::selectElement);
    connect(inlineOpenMBVMW, &OpenMBVGUI::MainWindow::objectDoubleClicked, this, [=](){ openElementEditor(); });
  }

  MainWindow::~MainWindow() {
    process.waitForFinished(-1);
    centralWidget()->layout()->removeWidget(inlineOpenMBVMW);
    delete inlineOpenMBVMW;
    // use nothrow boost::filesystem functions to avoid exceptions in this dtor
    boost::system::error_code ec;
    bfs::remove_all(uniqueTempDir, ec);
    bfs::remove("./.Project.mbsx", ec);
    delete project;
    parser->release();
    serializer->release();
    basicSerializer->release();
  }

  void MainWindow::setProjectChanged(bool changed) { 
    setWindowModified(changed);
    if(changed) {
      QSettings settings;
      xercesc::DOMDocument* oldDoc = static_cast<xercesc::DOMDocument*>(doc->cloneNode(true));
      oldDoc->setDocumentURI(doc->getDocumentURI());
      undos.push_back(oldDoc);
      if(undos.size() > maxUndo)
        undos.pop_front();
      redos.clear();
      if(allowUndo) actionUndo->setEnabled(true);
      actionRedo->setDisabled(true);
    }
  }
  
  bool MainWindow::maybeSave() {
    if(isWindowModified()) {
      QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Application"),
          tr("Project has been modified.\n"
            "Do you want to save your changes?"),
          QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
      if(ret == QMessageBox::Save) {
        if(actionSaveProject->isEnabled())
          return saveProject();
        else
          return saveProjectAs();
      } 
      else if(ret == QMessageBox::Cancel) 
        return false;
    }
    return true;
  }

  void MainWindow::openOptionsMenu(bool justSetOptions) {
    QSettings settings;
    OptionsDialog menu(this);
    menu.setAutoSave(settings.value("mainwindow/options/autosave", false).toBool());
    menu.setAutoSaveInterval(settings.value("mainwindow/options/autosaveinterval", 5).toInt());
    menu.setAutoExport(settings.value("mainwindow/options/autoexport", false).toBool());
    menu.setAutoExportDir(settings.value("mainwindow/options/autoexportdir", "./").toString());
    menu.setSaveStateVector(settings.value("mainwindow/options/savestatevector", false).toBool());
    menu.setMaxUndo(settings.value("mainwindow/options/maxundo", 10).toInt());
    menu.setShowFilters(settings.value("mainwindow/options/showfilters", false).toBool());
    menu.setAutoRefresh(settings.value("mainwindow/options/autorefresh", true).toBool());

#ifdef _WIN32
    QFile file(qgetenv("APPDATA")+"/mbsim-env/mbsimxml.modulepath");
#else
    QFile file(qgetenv("HOME")+"/.config/mbsim-env/mbsimxml.modulepath");
#endif
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    menu.setModulePath(file.readAll());
    file.close();

    int res = 1;
    if(!justSetOptions)
      res = menu.exec();
    if(res == 1) {
      settings.setValue("mainwindow/options/autosave",         menu.getAutoSave());
      settings.setValue("mainwindow/options/autosaveinterval", menu.getAutoSaveInterval());
      settings.setValue("mainwindow/options/autoexport",       menu.getAutoExport());
      settings.setValue("mainwindow/options/autoexportdir",    menu.getAutoExportDir());
      settings.setValue("mainwindow/options/savestatevector",  menu.getSaveStateVector());
      settings.setValue("mainwindow/options/maxundo",          menu.getMaxUndo());
      settings.setValue("mainwindow/options/showfilters",      menu.getShowFilters());
      settings.setValue("mainwindow/options/autorefresh",      menu.getAutoRefresh());

      file.open(QIODevice::WriteOnly | QIODevice::Text);
      file.write(menu.getModulePath().toUtf8());
      file.close();

      bool autoSave = menu.getAutoSave();
      int autoSaveInterval = menu.getAutoSaveInterval();
      if(not(menu.getSaveStateVector())) 
        actionSaveStateVectorAs->setDisabled(true);
      if(autoSave)
        autoSaveTimer.start(autoSaveInterval*60000);
      else
        autoSaveTimer.stop();
      maxUndo = menu.getMaxUndo();
      bool showFilters = menu.getShowFilters();
      elementViewFilter->setVisible(showFilters);
      parameterViewFilter->setVisible(showFilters);
      autoRefresh = menu.getAutoRefresh();
    }
  }

  void MainWindow::highlightObject(const QString &ID) {
    currentID = ID;
    inlineOpenMBVMW->highlightObject(ID.toStdString());
  }

  void MainWindow::selectionChanged(const QModelIndex &current) {
    if(allowUndo) {
      auto *model = static_cast<ElementTreeModel*>(elementView->model());
      auto *element=dynamic_cast<Element*>(model->getItem(current)->getItemData());
      if(element) {
        auto *emodel = static_cast<ParameterTreeModel*>(parameterView->model());
        vector<EmbedItemData*> parents = element->getEmbedItemParents();
        QModelIndex index = emodel->index(0,0);
        emodel->removeRow(index.row(), index.parent());
        if(!parents.empty()) {
          index = emodel->createParametersItem(parents[0]->getParameters());
          for(size_t i=0; i<parents.size()-1; i++)
            index = emodel->createParametersItem(parents[i+1]->getParameters(),index);
          emodel->createParametersItem(element->getParameters(),index);
        }
        else
          index = emodel->createParametersItem(element->getParameters());
        parameterView->expandAll();
        highlightObject(element->getID());
      }
      else
        highlightObject("");
    }
  }

  void MainWindow::elementViewClicked() {
    if(QApplication::mouseButtons()==Qt::RightButton) {
      QModelIndex index = elementView->selectionModel()->currentIndex();
      TreeItemData *itemData = static_cast<ElementTreeModel*>(elementView->model())->getItem(index)->getItemData();
      if(itemData) {
        QMenu *menu = itemData->createContextMenu();
        menu->exec(QCursor::pos());
        delete menu;
      } 
    }
    else if(QApplication::mouseButtons()==Qt::LeftButton)
      selectionChanged(elementView->selectionModel()->currentIndex());
  }

  void MainWindow::parameterViewClicked() {
    if(QApplication::mouseButtons()==Qt::RightButton) {
      QModelIndex index = parameterView->selectionModel()->currentIndex();
      auto *parameter = dynamic_cast<Parameter*>(static_cast<ParameterTreeModel*>(parameterView->model())->getItem(index)->getItemData());
      if(parameter) {
        QMenu *menu = parameter->createContextMenu();
        menu->exec(QCursor::pos());
        delete menu;
        return;
      }
      else {
        auto *item = static_cast<Parameters*>(static_cast<ParameterTreeModel*>(parameterView->model())->getItem(index)->getItemData());
        if(item) { // and item->getXMLElement()) {
          QMenu *menu = item->createContextMenu();
          menu->exec(QCursor::pos());
          delete menu;
        }
      }
    }
  }

  void MainWindow::solverViewClicked() {
    if(allowUndo) {
      auto *emodel = static_cast<ParameterTreeModel*>(parameterView->model());
      vector<EmbedItemData*> parents = getProject()->getSolver()->getEmbedItemParents();
      QModelIndex index = emodel->index(0,0);
      emodel->removeRow(index.row(), index.parent());
      if(!parents.empty()) {
        index = emodel->createParametersItem(parents[0]->getParameters());
        for(size_t i=0; i<parents.size()-1; i++)
          index = emodel->createParametersItem(parents[i+1]->getParameters(),index);
        emodel->createParametersItem(getProject()->getSolver()->getParameters(),index);
      }
      else
        index = emodel->createParametersItem(getProject()->getSolver()->getParameters());
      parameterView->expandAll();
      parameterView->scrollTo(index.child(emodel->rowCount(index)-1,0),QAbstractItemView::PositionAtTop);
    }
  }

  void MainWindow::projectViewClicked() {
    if(allowUndo) {
      auto *emodel = static_cast<ParameterTreeModel*>(parameterView->model());
      QModelIndex index = emodel->index(0,0);
      emodel->removeRow(index.row(), index.parent());
      emodel->createParametersItem(getProject()->getParameters());
      parameterView->expandAll();
    }
  }

  void MainWindow::newProject() {
    if(maybeSave()) {
      undos.clear();
      elementBuffer.first = NULL;
      parameterBuffer.first = NULL;
      setProjectChanged(false);
      actionOpenMBV->setDisabled(true);
      actionH5plotserie->setDisabled(true);
      actionEigenanalysis->setDisabled(true);
      actionFrequencyResponse->setDisabled(true);
      actionSaveDataAs->setDisabled(true);
      actionSaveMBSimH5DataAs->setDisabled(true);
      actionSaveOpenMBVDataAs->setDisabled(true);
      actionSaveStateVectorAs->setDisabled(true);
      actionSaveEigenanalysisAs->setDisabled(true);
      actionSaveProject->setDisabled(true);

      auto *pmodel = static_cast<ParameterTreeModel*>(parameterView->model());
      QModelIndex index = pmodel->index(0,0);
      pmodel->removeRows(index.row(), pmodel->rowCount(QModelIndex()), index.parent());

      auto *model = static_cast<ElementTreeModel*>(elementView->model());
      index = model->index(0,0);
      model->removeRow(index.row(), index.parent());

      delete project;

      doc = impl->createDocument();
      doc->setDocumentURI(X()%QUrl::fromLocalFile(QDir::currentPath()+"/Project.mbsx").toString().toStdString());

      project = new Project;
      project->createXMLElement(doc);
      projectView->setText(project->getName());

      model->createGroupItem(project->getDynamicSystemSolver(),QModelIndex());
      elementView->selectionModel()->setCurrentIndex(model->index(0,0), QItemSelectionModel::ClearAndSelect);

      solverView->setSolver(6);

      projectFile="";
      refresh();
      setWindowTitle("Project.mbsx[*]");
    }
  }

  void MainWindow::loadProject(const QString &file) {
    if(QFile::exists(file)) {
      undos.clear();
      elementBuffer.first = NULL;
      parameterBuffer.first = NULL;
      setProjectChanged(false);
      actionOpenMBV->setDisabled(true);
      actionH5plotserie->setDisabled(true);
      actionEigenanalysis->setDisabled(true);
      actionFrequencyResponse->setDisabled(true);
      actionSaveDataAs->setDisabled(true);
      actionSaveMBSimH5DataAs->setDisabled(true);
      actionSaveOpenMBVDataAs->setDisabled(true);
      actionSaveStateVectorAs->setDisabled(true);
      actionSaveEigenanalysisAs->setDisabled(true);
      actionSaveProject->setDisabled(false);
      projectFile = QDir::current().relativeFilePath(file);
      setCurrentProjectFile(file);
      try { 
        doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
      }
      catch(const std::exception &ex) {
        cout << ex.what() << endl;
        return;
      }
      catch(...) {
        cout << "Unknown exception." << endl;
        return;
      }
      setWindowTitle(projectFile+"[*]");
      rebuildTree();
      refresh();
    }
    else
      QMessageBox::warning(nullptr, "Project load", "Project file does not exist.");
  }

  void MainWindow::loadProject() {
    if(maybeSave()) {
      QString file=QFileDialog::getOpenFileName(this, "Open MBSim file", QFileInfo(getProjectFilePath()).absolutePath(), "MBSim files (*.mbsx);;XML files (*.xml);;All files (*.*)");
      if(file.startsWith("//"))
        file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
      if(not file.isEmpty())
        loadProject(file);
    }
  }

  bool MainWindow::saveProjectAs() {
    QString file=QFileDialog::getSaveFileName(this, "Save MBSim file", getProjectFilePath(), "MBSim files (*.mbsx)");
    if(not(file.isEmpty())) {
      file = file.endsWith(".mbsx")?file:file+".mbsx";
      doc->setDocumentURI(X()%QUrl::fromLocalFile(file).toString().toStdString());
      projectFile=QDir::current().relativeFilePath(file);
      setCurrentProjectFile(file);
      setWindowTitle(projectFile+"[*]");
      actionSaveProject->setDisabled(false);
      return saveProject();
    }
    return false;
  }

  bool MainWindow::saveProject(const QString &fileName, bool modifyStatus) {
    try {
      serializer->writeToURI(doc, X()%(fileName.isEmpty()?projectFile.toStdString():fileName.toStdString()));
      if(modifyStatus) setProjectChanged(false);
      return true;
    }
    catch(const std::exception &ex) {
      cout << ex.what() << endl;
    }
    catch(const DOMException &ex) {
      cout << X()%ex.getMessage() << endl;
    }
    catch(...) {
      cout << "Unknown exception." << endl;
    }
    return false;
  }

  void MainWindow::selectSolver(int i) {
    setProjectChanged(true);
    DOMNode *parent = getProject()->getSolver()->getXMLElement()->getParentNode();
    getProject()->getSolver()->removeXMLElement(false);
    getProject()->setSolver(solverView->createSolver(i));
    getProject()->getSolver()->createXMLElement(parent);
    std::vector<Parameter*> param;
    DOMElement *ele = MBXMLUtils::E(static_cast<DOMElement*>(parent))->getFirstElementChildNamed(MBXMLUtils::PV%"Parameter");
    if(ele) param = Parameter::createParameters(ele);
    for(auto & i : param)
      getProject()->getSolver()->addParameter(i);
    solverViewClicked();
  }

  // update model parameters including additional paramters from paramList
  void MainWindow::updateParameters(EmbedItemData *item, bool exceptLatestParameter) {
    shared_ptr<xercesc::DOMDocument> doc=mbxmlparser->createDocument();
    doc->setDocumentURI(this->doc->getDocumentURI());
    DOMElement *eleE0 = D(doc)->createElement(PV%"Embed");
    doc->insertBefore(eleE0,nullptr);
    DOMElement *eleP0 = D(doc)->createElement(PV%"Parameter");
    eleE0->insertBefore(eleP0,nullptr);
    DOMElement *eleE1 = D(doc)->createElement(PV%"Embed");
    eleE0->insertBefore(eleE1,nullptr);
    DOMElement *eleP1 = D(doc)->createElement(PV%"Parameter");
    eleE1->insertBefore(eleP1,nullptr);
    string evalName="octave"; // default evaluator
    if(project)
      evalName = project->getEvaluator();
    else {
      DOMElement *root = this->doc->getDocumentElement();
      DOMElement *evaluator;
      if(E(root)->getTagName()==PV%"Embed") {
        auto r=root->getFirstElementChild();
        if(E(r)->getTagName()==PV%"Parameter")
          r=r->getNextElementSibling();
        evaluator=E(r)->getFirstElementChildNamed(PV%"evaluator");
      }
      else
        evaluator=E(root)->getFirstElementChildNamed(PV%"evaluator");
      if(evaluator)
        evalName=X()%E(evaluator)->getFirstTextChild()->getData();
    }
    eval=Eval::createEvaluator(evalName, &dependencies);
    if(item) {
      vector<EmbedItemData*> parents = item->getEmbedItemParents();
      for(auto & parent : parents) {
        for(size_t j=0; j<parent->getNumberOfParameters(); j++) {
          DOMNode *node = doc->importNode(parent->getParameter(j)->getXMLElement(),true);
          eleP0->insertBefore(node,nullptr);
          boost::filesystem::path orgFileName=E(parent->getParameter(j)->getXMLElement())->getOriginalFilename();
          DOMProcessingInstruction *filenamePI=node->getOwnerDocument()->createProcessingInstruction(X()%"OriginalFilename",
              X()%orgFileName.string());
          node->insertBefore(filenamePI, node->getFirstChild());
        }
      }
      for(int j=0; j<item->getNumberOfParameters()-exceptLatestParameter; j++) {
        DOMNode *node = doc->importNode(item->getParameter(j)->getXMLElement(),true);
        eleP1->insertBefore(node,nullptr);
        boost::filesystem::path orgFileName=E(item->getParameter(j)->getXMLElement())->getOriginalFilename();
        DOMProcessingInstruction *filenamePI=node->getOwnerDocument()->createProcessingInstruction(X()%"OriginalFilename",
            X()%orgFileName.string());
        node->insertBefore(filenamePI, node->getFirstChild());
      }
      try {
        D(doc)->validate();
        DOMElement *ele = doc->getDocumentElement()->getFirstElementChild();
        eval->addParamSet(ele);
        if(item->getXMLElement()) {
          string counterName = item->getEmbedXMLElement()?E(item->getEmbedXMLElement())->getAttribute("counterName"):"";
          if(not counterName.empty())
            eval->addParam(eval->cast<string>(eval->stringToValue(counterName,item->getEmbedXMLElement(),false)),eval->create(1.0));
        }
        ele = ele->getNextElementSibling()->getFirstElementChild();
        eval->addParamSet(ele);
      }
      catch(const std::exception &error) {
        cout << string("An exception occurred in updateParameters: ") + error.what() << endl;
      }
      catch(...) {
        cout << "An unknown exception occurred in updateParameters." << endl;
      }
    }
  }

  void MainWindow::saveDataAs() {
    QString dir = QFileDialog::getExistingDirectory (this, "Export simulation data", getProjectPath());
    if(dir != "") {
      QDir directory(dir);
      QMessageBox::StandardButton ret = QMessageBox::Ok;
      if(directory.count()>2)
        ret = QMessageBox::warning(this, tr("Application"), tr("Directory not empty. Overwrite existing files?"), QMessageBox::Ok | QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) {
        QSettings settings;
        saveMBSimH5Data(dir+"/MBS.mbsh5");
        saveOpenMBVXMLData(dir+"/MBS.ombvx");
        saveOpenMBVH5Data(dir+"/MBS.ombvh5");
        saveEigenanalysis(dir+"/MBS.eigenanalysis.mat");
        if(settings.value("mainwindow/options/savestatevector", false).toBool())
          saveStateVector(dir+"/statevector.asc");
      }
    }
  }

  void MainWindow::saveMBSimH5DataAs() {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = model->index(0,0);
    QString file=QFileDialog::getSaveFileName(this, "Export MBSim H5 file", getProjectDir().absoluteFilePath(model->getItem(index)->getItemData()->getName()+".mbsh5"), "H5 files (*.mbsh5)");
    if(file!="") {
      saveMBSimH5Data(file);
    }
  }

  void MainWindow::saveMBSimH5Data(const QString &file) {
    if(QFile::exists(file))
      QFile::remove(file);
    QFile::copy(QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".mbsh5",file);
  }

  void MainWindow::saveOpenMBVDataAs() {
    QString dir = QFileDialog::getExistingDirectory(this, "Export OpenMBV data", getProjectPath());
    if(dir != "") {
      QDir directory(dir);
      QMessageBox::StandardButton ret = QMessageBox::Ok;
      if(directory.count()>2)
        ret = QMessageBox::warning(this, tr("Application"), tr("Directory not empty. Overwrite existing files?"), QMessageBox::Ok | QMessageBox::Cancel);
      if(ret == QMessageBox::Ok) {
        saveOpenMBVXMLData(dir+"/MBS.ombvx");
        saveOpenMBVH5Data(dir+"/MBS.ombvh5");
      }
    }
  }

  void MainWindow::saveOpenMBVXMLData(const QString &file) {
    if(QFile::exists(file))
      QFile::remove(file);
    QFile::copy(QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".ombvx",file);
  }

  void MainWindow::saveOpenMBVH5Data(const QString &file) {
    if(QFile::exists(file))
      QFile::remove(file);
    QFile::copy(QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".ombvh5",file);
  }

  void MainWindow::saveStateVectorAs() {
    QString file=QFileDialog::getSaveFileName(this, "Export state vector file", getProjectDir().absoluteFilePath("statevector.asc"), "ASCII files (*.asc)");
    if(file!="") {
      saveStateVector(file);
    }
  }

  void MainWindow::saveStateVector(const QString &file) {
    if(QFile::exists(file))
      QFile::remove(file);
    QFile::copy(QString::fromStdString(uniqueTempDir.generic_string())+"/statevector.asc",file);
  }

  void MainWindow::saveEigenanalysisAs() {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = model->index(0,0);
    QString file=QFileDialog::getSaveFileName(this, "Export eigenanalysis file", getProjectDir().absoluteFilePath(model->getItem(index)->getItemData()->getName()+".eigenanalysis.mat"), "mat files (*.eigenanalysis.mat)");
    if(file!="") {
      saveEigenanalysis(file);
    }
  }

  void MainWindow::saveEigenanalysis(const QString &file) {
    if(QFile::exists(file))
      QFile::remove(file);
    QFile::copy(QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".eigenanalysis.mat",file);
  }

  void MainWindow::mbsimxml(int task) {
    currentTask = task;

    shared_ptr<xercesc::DOMDocument> doc=mbxmlparser->createDocument();
    doc->setDocumentURI(this->doc->getDocumentURI());
    auto *newDocElement = static_cast<DOMElement*>(doc->importNode(this->doc->getDocumentElement(), true));
    doc->insertBefore(newDocElement, nullptr);
    getProject()->processIDAndHref(newDocElement);

    QString uniqueTempDir_ = QString::fromStdString(uniqueTempDir.generic_string());
    QString projectFile;

    projectFile=uniqueTempDir_+"/Project.flat.mbsx";

    actionSimulate->setDisabled(true);
    actionRefresh->setDisabled(true);
    if(task==0) {
      actionSaveDataAs->setDisabled(true);
      actionSaveMBSimH5DataAs->setDisabled(true);
      actionSaveOpenMBVDataAs->setDisabled(true);
      actionSaveStateVectorAs->setDisabled(true);
      actionSaveEigenanalysisAs->setDisabled(true);
      actionOpenMBV->setDisabled(true);
      actionH5plotserie->setDisabled(true);
      actionEigenanalysis->setDisabled(true);
      actionFrequencyResponse->setDisabled(true);
    }
    else if(task==1) {
      if(OpenMBVGUI::MainWindow::getInstance()->getObjectList()->invisibleRootItem()->childCount())
        static_cast<OpenMBVGUI::Group*>(OpenMBVGUI::MainWindow::getInstance()->getObjectList()->invisibleRootItem()->child(0))->unloadFileSlot();
    }

    echoView->clearOutput();
    DOMElement *root;
    QString errorText;

    *debugStreamFlag=echoView->debugEnabled();

    try {
      fmatvec::Atom::msgStatic(fmatvec::Atom::Info)<<"Validate "<<D(doc)->getDocumentFilename().string()<<endl;
      D(doc)->validate();
      root = doc->getDocumentElement();
      vector<boost::filesystem::path> dependencies;
      shared_ptr<Eval> eval=Eval::createEvaluator(project->getEvaluator(), &dependencies);
      Preprocess::preprocess(mbxmlparser, eval, dependencies, root);
    }
    catch(exception &ex) {
      errorText = ex.what();
    }
    catch(...) {
      errorText = "Unknown error";
    }
    if(not errorText.isEmpty()) {
      echoView->addOutputText("<span class=\"MBSIMGUI_ERROR\">"+errorText+"</span>");
      echoView->updateOutput(true);
      actionSimulate->setDisabled(false);
      actionRefresh->setDisabled(false);
      actionDebug->setDisabled(false);
      statusBar()->showMessage(tr("Ready"));
      return;
    }
    echoView->updateOutput(true);
    // adapt the evaluator in the dom
    DOMElement *evaluator=E(root)->getFirstElementChildNamed(PV%"evaluator");
    if(evaluator)
      E(evaluator)->getFirstTextChild()->setData(X()%"xmlflat");
    else {
      evaluator=D(doc)->createElement(PV%"evaluator");
      evaluator->appendChild(doc->createTextNode(X()%"xmlflat"));
      root->insertBefore(evaluator, root->getFirstChild());
    }
    E(root)->setOriginalFilename();
    basicSerializer->writeToURI(doc.get(), X()%projectFile.toStdString());
    QStringList arg;
    QSettings settings;
    if(currentTask==1)
      arg.append("--stopafterfirststep");
    else if(settings.value("mainwindow/options/savestatevector", false).toBool())
      arg.append("--savefinalstatevector");

    // we print everything except status messages to stdout
    arg.append("--stdout"); arg.append(R"#(info~<span class="MBSIMGUI_INFO">~</span>)#");
    arg.append("--stdout"); arg.append(R"#(warn~<span class="MBSIMGUI_WARN">~</span>)#");
    if(*debugStreamFlag) {
      arg.append("--stdout"); arg.append(R"#(debug~<span class="MBSIMGUI_DEBUG">~</span>)#");
    }
    arg.append("--stdout"); arg.append(R"#(error~<span class="MBSIMGUI_ERROR">~</span>)#");
    arg.append("--stdout"); arg.append(R"#(depr~<span class="MBSIMGUI_DEPRECATED">~</span>)#");
    // status message go to stderr
    arg.append("--stderr"); arg.append("status~~\n");

    arg.append(projectFile);
    process.setWorkingDirectory(uniqueTempDir_);
    process.start(QString::fromStdString((installPath/"bin"/"mbsimflatxml").string()), arg);
  }

  void MainWindow::simulate() {
    statusBar()->showMessage(tr("Simulate"));
    mbsimxml(0);
  }

  void MainWindow::refresh() {
    statusBar()->showMessage(tr("Refresh"));
    mbsimxml(1);
  }

  void MainWindow::openmbv() {
    QString name = QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".ombvx";
    if(QFile::exists(name)) {
      QStringList arg;
      arg.append("--autoreload");
      arg.append(name);
      QProcess::startDetached(QString::fromStdString((installPath/"bin"/"openmbv").string()), arg);
    }
  }

  void MainWindow::h5plotserie() {
    QString name = QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".mbsh5";
    if(QFile::exists(name)) {
      QStringList arg;
      arg.append(name);
      QProcess::startDetached(QString::fromStdString((installPath/"bin"/"h5plotserie").string()), arg);
    }
  }

  void MainWindow::eigenanalysis() {
    QString name = QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".eigenanalysis.mat";
    if(QFile::exists(name)) {
      EigenanalysisDialog *dialog = new EigenanalysisDialog(name,this);
      dialog->show();
    }
  }

  void MainWindow::frequencyResponse() {
    QString name = QString::fromStdString(uniqueTempDir.generic_string())+"/"+project->getDynamicSystemSolver()->getName()+".harmonic_response_analysis.mat";
    if(QFile::exists(name)) {
      HarmonicResponseDialog *dialog = new HarmonicResponseDialog(name,this);
      dialog->show();
    }
  }

  void MainWindow::debug() {
    currentTask = 0;
    QString uniqueTempDir_ = QString::fromStdString(uniqueTempDir.generic_string());
    QString projectFile = uniqueTempDir_+"/Project.mbsx";
    serializer->writeToURI(doc, X()%projectFile.toStdString());
    QStringList arg;
    arg.append("--stopafterfirststep");
    arg.append(projectFile);
    echoView->clearOutput();
    process.setWorkingDirectory(uniqueTempDir_);
    process.start(QString::fromStdString((installPath/"bin"/"mbsimxml").string()), arg);
  }

  void MainWindow::selectElement(const string& ID) {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    auto it=model->idEleMap.find(QString::fromStdString(ID));
    if(it!=model->idEleMap.end())
      elementView->selectionModel()->setCurrentIndex(it->second,QItemSelectionModel::ClearAndSelect);
  }

  void MainWindow::help() {
    QMessageBox::information(this, tr("MBSimGUI - Help"), tr("<p>Please visit <a href=\"https://www.mbsim-env.de\">MBSim-Environment</a> for documentation.</p>"));
  }

  void MainWindow::xmlHelp(const QString &url) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString((installPath/"share"/"mbxmlutils"/"doc"/"http___www_mbsim-env_de_MBSimXML"/"mbsimxml.html").string())));
  }

  void MainWindow::about() {
     QMessageBox::about(this, tr("About MBSimGUI"), (tr("<p><b>MBSimGUI %1</b></p><p>MBSimGUI is a graphical user interface for the multibody simulation software MBSim.</p>").arg(VERSION)
           + tr("<p>See <a href=\"https://www.mbsim-env.de\">MBSim-Environment</a> for more information.</p>"
             "<p>Copyright &copy; Martin Foerg <tt>&lt;martin.o.foerg@googlemail.com&gt;</tt><p/>"
             "<p>Licensed under the General Public License (see file COPYING).</p>"
             "<p>This is free software; see the source for copying conditions. There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.</p>")));
  }

  void MainWindow::rebuildTree() {

    auto *pmodel = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = pmodel->index(0,0);
    pmodel->removeRows(index.row(), pmodel->rowCount(QModelIndex()), index.parent());

    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    index = model->index(0,0);
    model->removeRow(index.row(), index.parent());

    delete project;
    project = 0;

    project=Embed<Project>::create(doc->getDocumentElement(),nullptr);
    project->create();
    projectView->setText(project->getName());

    model->createGroupItem(project->getDynamicSystemSolver());
    elementView->selectionModel()->setCurrentIndex(model->index(0,0), QItemSelectionModel::ClearAndSelect);

    solverView->setSolver(getProject()->getSolver());
  }

  void MainWindow::edit() {
    if(projectView->hasFocus())
      openProjectEditor();
    else if(elementView->hasFocus())
      openElementEditor();
    else if(parameterView->hasFocus())
      openParameterEditor();
    else if(solverView->hasFocus())
      openSolverEditor();
  }

  void MainWindow::undo() {
    elementBuffer.first = NULL;
    parameterBuffer.first = NULL;
    setWindowModified(true);
    redos.push_back(doc);
    doc = undos.back();
    undos.pop_back();
    rebuildTree();
    if(getAutoRefresh()) refresh();
    actionUndo->setDisabled(undos.empty());
    actionRedo->setEnabled(true);
  }

  void MainWindow::redo() {
    undos.push_back(doc);
    doc = redos.back();
    redos.pop_back();
    rebuildTree();
    if(getAutoRefresh()) refresh();
    actionRedo->setDisabled(redos.empty());
    actionUndo->setEnabled(true);
  }

  void MainWindow::removeElement() {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    auto *element = dynamic_cast<Element*>(model->getItem(index)->getItemData());
    if(element and (not dynamic_cast<DynamicSystemSolver*>(element)) and (not dynamic_cast<InternalFrame*>(element))) {
      setProjectChanged(true);
      if(element == elementBuffer.first)
        elementBuffer.first = NULL;
      element->removeXMLElement();
      element->getParent()->removeElement(element);
      model->removeRow(index.row(), index.parent());
      if(getAutoRefresh()) refresh();
    }
  }

  void MainWindow::removeParameter() {
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *parameter = dynamic_cast<Parameter*>(model->getItem(index)->getItemData());
    if(parameter) {
      setProjectChanged(true);
      if(parameter == parameterBuffer.first)
        parameterBuffer.first = NULL;
      DOMNode *ps = parameter->getXMLElement()->getPreviousSibling();
      if(ps and X()%ps->getNodeName()=="#text")
        parameter->getXMLElement()->getParentNode()->removeChild(ps);
      parameter->getXMLElement()->getParentNode()->removeChild(parameter->getXMLElement());
      parameter->getParent()->removeParameter(parameter);
      parameter->getParent()->maybeRemoveEmbedXMLElement();
      model->removeRow(index.row(), index.parent());
      if(getAutoRefresh()) refresh();
    }
  }

  void MainWindow::remove() {
    if(elementView->hasFocus())
      removeElement();
    else if(parameterView->hasFocus())
      removeParameter();
  }

  void MainWindow::copy(bool cut) {
    if(elementView->hasFocus())
      copyElement(cut);
    else if(parameterView->hasFocus())
      copyParameter(cut);
  }

  void MainWindow::paste() {
    if(elementView->hasFocus()) {
      auto *model = static_cast<ElementTreeModel*>(elementView->model());
      QModelIndex index = elementView->selectionModel()->currentIndex();
      ContainerItemData *item = dynamic_cast<FrameItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Frame*>(getElementBuffer().first)) {
        loadFrame(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<ContourItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Contour*>(getElementBuffer().first)) {
        loadContour(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<GroupItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Group*>(getElementBuffer().first)) {
        loadGroup(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<ObjectItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Object*>(getElementBuffer().first)) {
        loadObject(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<LinkItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Link*>(getElementBuffer().first)) {
        loadLink(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<ConstraintItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Constraint*>(getElementBuffer().first)) {
        loadConstraint(item->getElement(),getElementBuffer().first);
        return;
      }
      item = dynamic_cast<ObserverItemData*>(model->getItem(index)->getItemData());
      if(item and dynamic_cast<Observer*>(getElementBuffer().first)) {
        loadObserver(item->getElement(),getElementBuffer().first);
        return;
      }
    }
    else if(parameterView->hasFocus()) {
      auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
      QModelIndex index = parameterView->selectionModel()->currentIndex();
      auto *item = dynamic_cast<Parameters*>(model->getItem(index)->getItemData());
      if(item)
        loadParameter(item->getItem(),getParameterBuffer().first);
    }
  }

  void MainWindow::move(bool up) {
    if(elementView->hasFocus()) {
      auto *model = static_cast<ElementTreeModel*>(elementView->model());
      QModelIndex index = elementView->selectionModel()->currentIndex();
      auto *frame = dynamic_cast<Frame*>(model->getItem(index)->getItemData());
      if(frame and (not dynamic_cast<InternalFrame*>(frame)) and (up?(frame->getParent()->getIndexOfFrame(frame)>1):(frame->getParent()->getIndexOfFrame(frame)<frame->getParent()->getNumberOfFrames()-1))) {
        moveFrame(up);
        return;
      }
      auto *contour = dynamic_cast<Contour*>(model->getItem(index)->getItemData());
      if(contour and (up?(contour->getParent()->getIndexOfContour(contour)>0):(contour->getParent()->getIndexOfContour(contour)<contour->getParent()->getNumberOfContours()-1))) {
        moveContour(up);
        return;
      }
      auto *group = dynamic_cast<Group*>(model->getItem(index)->getItemData());
      if(group and (up?(group->getParent()->getIndexOfGroup(group)>0):(group->getParent()->getIndexOfGroup(group)<group->getParent()->getNumberOfGroups()-1))) {
        moveGroup(up);
        return;
      }
      auto *object = dynamic_cast<Object*>(model->getItem(index)->getItemData());
      if(object and (up?(object->getParent()->getIndexOfObject(object)>0):(object->getParent()->getIndexOfObject(object)<object->getParent()->getNumberOfObjects()-1))) {
        moveObject(up);
        return;
      }
      auto *link = dynamic_cast<Link*>(model->getItem(index)->getItemData());
      if(link and (up?(link->getParent()->getIndexOfLink(link)>0):(link->getParent()->getIndexOfLink(link)<link->getParent()->getNumberOfLinks()-1))) {
        moveLink(up);
        return;
      }
      auto *constraint = dynamic_cast<Constraint*>(model->getItem(index)->getItemData());
      if(constraint and (up?(constraint->getParent()->getIndexOfConstraint(constraint)>0):(constraint->getParent()->getIndexOfConstraint(constraint)<constraint->getParent()->getNumberOfConstraints()-1))) {
        moveConstraint(up);
        return;
      }
      auto *observer = dynamic_cast<Observer*>(model->getItem(index)->getItemData());
      if(observer and (up?(observer->getParent()->getIndexOfObserver(observer)>0):(observer->getParent()->getIndexOfObserver(observer)<observer->getParent()->getNumberOfObservers()-1))) {
        moveObserver(up);
        return;
      }
    }
    else if(parameterView->hasFocus()) {
      auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
      QModelIndex index = parameterView->selectionModel()->currentIndex();
      auto *parameter = dynamic_cast<Parameter*>(model->getItem(index)->getItemData());
      if(parameter and (up?(parameter->getParent()->getIndexOfParameter(parameter)>0):(parameter->getParent()->getIndexOfParameter(parameter)<parameter->getParent()->getNumberOfParameters()-1)))
        moveParameter(up);
    }
  }

  void MainWindow::copyElement(bool cut) {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    auto *element = static_cast<Element*>(model->getItem(index)->getItemData());
    if((not dynamic_cast<DynamicSystemSolver*>(element)) and (not dynamic_cast<InternalFrame*>(element)))
      elementBuffer = make_pair(element,cut);
  }

  void MainWindow::copyParameter(bool cut) {
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *parameter = static_cast<Parameter*>(model->getItem(index)->getItemData());
    parameterBuffer = make_pair(parameter,cut);
  }

  void MainWindow::moveParameter(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *parameter = static_cast<Parameter*>(model->getItem(index)->getItemData());
    int i = parameter->getParent()->getIndexOfParameter(parameter);
    int j = up?i-1:i+1;
    DOMElement *tmp = up?parameter->getXMLElement()->getPreviousElementSibling():parameter->getXMLElement()->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = parameter->getXMLElement()->getParentNode();
    DOMNode *ps = parameter->getXMLElement()->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    DOMNode *ele = parent->removeChild(parameter->getXMLElement());
    parent->insertBefore(ele,tmp);
    parameter->getParent()->setParameter(parameter->getParent()->getParameter(j),i);
    parameter->getParent()->setParameter(parameter,j);
    model->removeRows(0,parameter->getParent()->getNumberOfParameters(),parentIndex);
    for(int i=0; i<parameter->getParent()->getNumberOfParameters(); i++)
      model->createParameterItem(parameter->getParent()->getParameter(i),parentIndex);
    parameterView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveFrame(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *frame = static_cast<Frame*>(model->getItem(index)->getItemData());
    int i = frame->getParent()->getIndexOfFrame(frame);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(frame->getEmbedXMLElement()?frame->getEmbedXMLElement():frame->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    frame->getParent()->setFrame(frame->getParent()->getFrame(j),i);
    frame->getParent()->setFrame(frame,j);
    model->removeRows(0,frame->getParent()->getNumberOfFrames(),parentIndex);
    for(int i=0; i<frame->getParent()->getNumberOfFrames(); i++)
      model->createFrameItem(frame->getParent()->getFrame(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveContour(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *contour = static_cast<Contour*>(model->getItem(index)->getItemData());
    int i = contour->getParent()->getIndexOfContour(contour);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(contour->getEmbedXMLElement()?contour->getEmbedXMLElement():contour->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    contour->getParent()->setContour(contour->getParent()->getContour(j),i);
    contour->getParent()->setContour(contour,j);
    model->removeRows(0,contour->getParent()->getNumberOfContours(),parentIndex);
    for(int i=0; i<contour->getParent()->getNumberOfContours(); i++)
      model->createContourItem(contour->getParent()->getContour(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveGroup(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *group = static_cast<Group*>(model->getItem(index)->getItemData());
    int i = group->getParent()->getIndexOfGroup(group);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(group->getEmbedXMLElement()?group->getEmbedXMLElement():group->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    group->getParent()->setGroup(group->getParent()->getGroup(j),i);
    group->getParent()->setGroup(group,j);
    model->removeRows(0,group->getParent()->getNumberOfGroups(),parentIndex);
    for(int i=0; i<group->getParent()->getNumberOfGroups(); i++)
      model->createGroupItem(group->getParent()->getGroup(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveObject(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *object = static_cast<Object*>(model->getItem(index)->getItemData());
    int i = object->getParent()->getIndexOfObject(object);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(object->getEmbedXMLElement()?object->getEmbedXMLElement():object->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    object->getParent()->setObject(object->getParent()->getObject(j),i);
    object->getParent()->setObject(object,j);
    model->removeRows(0,object->getParent()->getNumberOfObjects(),parentIndex);
    for(int i=0; i<object->getParent()->getNumberOfObjects(); i++)
      model->createObjectItem(object->getParent()->getObject(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveLink(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *link = static_cast<Link*>(model->getItem(index)->getItemData());
    int i = link->getParent()->getIndexOfLink(link);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(link->getEmbedXMLElement()?link->getEmbedXMLElement():link->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    link->getParent()->setLink(link->getParent()->getLink(j),i);
    link->getParent()->setLink(link,j);
    model->removeRows(0,link->getParent()->getNumberOfLinks(),parentIndex);
    for(int i=0; i<link->getParent()->getNumberOfLinks(); i++)
      model->createLinkItem(link->getParent()->getLink(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveConstraint(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *constraint = static_cast<Constraint*>(model->getItem(index)->getItemData());
    int i = constraint->getParent()->getIndexOfConstraint(constraint);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(constraint->getEmbedXMLElement()?constraint->getEmbedXMLElement():constraint->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    constraint->getParent()->setConstraint(constraint->getParent()->getConstraint(j),i);
    constraint->getParent()->setConstraint(constraint,j);
    model->removeRows(0,constraint->getParent()->getNumberOfConstraints(),parentIndex);
    for(int i=0; i<constraint->getParent()->getNumberOfConstraints(); i++)
      model->createConstraintItem(constraint->getParent()->getConstraint(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::moveObserver(bool up) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    QModelIndex parentIndex = index.parent();
    auto *observer = static_cast<Observer*>(model->getItem(index)->getItemData());
    int i = observer->getParent()->getIndexOfObserver(observer);
    int j = up?i-1:i+1;
    DOMElement *ele = static_cast<DOMElement*>(observer->getEmbedXMLElement()?observer->getEmbedXMLElement():observer->getXMLElement());
    DOMElement *tmp = up?ele->getPreviousElementSibling():ele->getNextElementSibling()->getNextElementSibling();
    DOMNode *parent = ele->getParentNode();
    DOMNode *ps = ele->getPreviousSibling();
    if(ps and X()%ps->getNodeName()=="#text")
      parent->removeChild(ps);
    parent->removeChild(ele);
    parent->insertBefore(ele,tmp);
    observer->getParent()->setObserver(observer->getParent()->getObserver(j),i);
    observer->getParent()->setObserver(observer,j);
    model->removeRows(0,observer->getParent()->getNumberOfObservers(),parentIndex);
    for(int i=0; i<observer->getParent()->getNumberOfObservers(); i++)
      model->createObserverItem(observer->getParent()->getObserver(i),parentIndex);
    elementView->setCurrentIndex(parentIndex.child(j,0));
  }

  void MainWindow::saveElementAs() {
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    auto *element = static_cast<Element*>(model->getItem(index)->getItemData());
    bool includeParameter = false;
    if(element->getEmbedXMLElement()) {
      SaveDialog saveDialog(this);
      saveDialog.exec();
      includeParameter = saveDialog.includeParameter();
    }
    QString file=QFileDialog::getSaveFileName(this, "XML model files", getProjectDir().absoluteFilePath(element->getName()+".mbsimele.xml"), "XML files (*.xml)");
    if(not file.isEmpty()) {
      xercesc::DOMDocument *edoc = impl->createDocument();
      DOMNode *node = edoc->importNode(includeParameter?element->getEmbedXMLElement():element->getXMLElement(),true);
      edoc->insertBefore(node,nullptr);
      serializer->writeToURI(edoc, X()%file.toStdString());
    }
  }

  void MainWindow::enableElement(bool enabled) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
    auto *element = static_cast<Element*>(model->getItem(index)->getItemData());
    DOMElement *embedNode = element->getEmbedXMLElement();
    if(enabled) {
      if(element->getNumberOfParameters())
        E(embedNode)->setAttribute("count","1");
      else {
        E(embedNode)->removeAttribute("count");
        E(embedNode)->removeAttribute("counterName");
      }
    }
    else {
      if(not embedNode)
        embedNode = element->createEmbedXMLElement();
      E(embedNode)->setAttribute("counterName","n");
      E(embedNode)->setAttribute("count","0");
    }
    element->maybeRemoveEmbedXMLElement();
    element->updateStatus();
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::saveSolverAs() {
    Solver *solver = project->getSolver();
    bool includeParameter = false;
    if(solver->getEmbedXMLElement()) {
      SaveDialog saveDialog(this);
      saveDialog.exec();
      includeParameter = saveDialog.includeParameter();
    }
    QString file=QFileDialog::getSaveFileName(this, "XML model files", getProjectDir().absoluteFilePath(solver->getName()+".mbsimslv.xml"), "XML files (*.xml)");
    if(not file.isEmpty()) {
      xercesc::DOMDocument *edoc = impl->createDocument();
      DOMNode *node = edoc->importNode(includeParameter?solver->getEmbedXMLElement():solver->getXMLElement(),true);
      edoc->insertBefore(node,nullptr);
      serializer->writeToURI(edoc, X()%file.toStdString());
    }
  }

  void MainWindow::saveParametersAs() {
    ParameterTreeModel *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *item = static_cast<Parameters*>(model->getItem(index)->getItemData());
    QString file=QFileDialog::getSaveFileName(this, "XML model files", getProjectDir().absoluteFilePath(item->getItem()->getName()+".mbsimembed.xml"), "XML files (*.xml)");
    if(not file.isEmpty()) {
      xercesc::DOMDocument *edoc = impl->createDocument();
      DOMNode *node;
      node = edoc->importNode(item->getItem()->getEmbedXMLElement()->getFirstElementChild(),true);
      edoc->insertBefore(node,nullptr);
      serializer->writeToURI(edoc, X()%file.toStdString());
    }
  }

  void MainWindow::addFrame(Frame *frame, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    frame->setName(frame->getName()+toQStr(model->getItem(index)->getID()));
    parent->addFrame(frame);
    frame->createXMLElement(parent->getXMLFrames());
    model->createFrameItem(frame,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addContour(Contour *contour, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    contour->setName(contour->getName()+toQStr(model->getItem(index)->getID()));
    parent->addContour(contour);
    contour->createXMLElement(parent->getXMLContours());
    model->createContourItem(contour,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addGroup(Group *group, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    group->setName(group->getName()+toQStr(model->getItem(index)->getID()));
    parent->addGroup(group);
    group->createXMLElement(parent->getXMLGroups());
    model->createGroupItem(group,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addObject(Object *object, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    object->setName(object->getName()+toQStr(model->getItem(index)->getID()));
    parent->addObject(object);
    object->createXMLElement(parent->getXMLObjects());
    model->createObjectItem(object,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addLink(Link *link, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    link->setName(link->getName()+toQStr(model->getItem(index)->getID()));
    parent->addLink(link);
    link->createXMLElement(parent->getXMLLinks());
    model->createLinkItem(link,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addConstraint(Constraint *constraint, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    constraint->setName(constraint->getName()+toQStr(model->getItem(index)->getID()));
    parent->addConstraint(constraint);
    constraint->createXMLElement(parent->getXMLConstraints());
    model->createConstraintItem(constraint,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addObserver(Observer *observer, Element *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    QModelIndex index = elementView->selectionModel()->currentIndex();
//    observer->setName(observer->getName()+toQStr(model->getItem(index)->getID()));
    parent->addObserver(observer);
    observer->createXMLElement(parent->getXMLObservers());
    model->createObserverItem(observer,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    openElementEditor(false);
  }

  void MainWindow::addParameter(Parameter *parameter, EmbedItemData *parent) {
    setProjectChanged(true);
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    parent->addParameter(parameter);
    parameter->createXMLElement(parent->createParameterXMLElement());
//    if(parameter->getName()!="import")
//      parameter->setName(parameter->getName()+toQStr(model->getItem(index)->getID()));
    QModelIndex newIndex = model->createParameterItem(parameter,index);
    parameterView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::ClearAndSelect);
    openParameterEditor(false);
  }

  void MainWindow::loadParameter(EmbedItemData *parent, Parameter *param, bool embed) {
    setProjectChanged(true);
    vector<DOMElement*> elements;
    QString file;
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    if(param) {
      elements.push_back(static_cast<DOMElement*>(doc->importNode(param->getXMLElement(),true)));
      if(parameterBuffer.second) {
        parameterBuffer.first = NULL;
        DOMNode *ps = param->getXMLElement()->getPreviousSibling();
        if(ps and X()%ps->getNodeName()=="#text")
          param->getXMLElement()->getParentNode()->removeChild(ps);
        param->getXMLElement()->getParentNode()->removeChild(param->getXMLElement());
        param->getParent()->removeParameter(param);
        QModelIndex index = model->findItem(param,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML frame files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        DOMElement *ele = embed?doc->getDocumentElement()->getFirstElementChild():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true))->getFirstElementChild();
        while(ele) {
          elements.push_back(ele);
          ele = ele->getNextElementSibling();
        }
      }
      else
        return;
    }
    QModelIndex newIndex;
    for(auto & element : elements) {
      Parameter *parameter=ObjectFactory::getInstance()->createParameter(element);
      if(not parameter) {
        QMessageBox::warning(nullptr, "Import", "Cannot import file.");
        return;
      }
      if(embed) {
        parent->setEmbededParameters(true);
        E(parent->createEmbedXMLElement())->setAttribute("parameterHref",getProjectDir().relativeFilePath(file).toStdString());
      } 
      else
        parent->createParameterXMLElement()->insertBefore(element,nullptr);
      parameter->setXMLElement(element);
      parent->addParameter(parameter);
      newIndex = model->createParameterItem(parameter,index);
    }
    parameterView->selectionModel()->setCurrentIndex(newIndex, QItemSelectionModel::ClearAndSelect);
  }

  void MainWindow::removeParameter(EmbedItemData *parent) {
    setProjectChanged(true);
    auto *model = static_cast<ParameterTreeModel*>(parameterView->model());
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    int n = parent->getNumberOfParameters();
    if(parent->getEmbededParameters()) {
      for(int i=n-1; i>=0; i--) {
        parent->removeParameter(parent->getParameter(i));
      }
      parent->getEmbedXMLElement()->removeAttribute(X()%"parameterHref");
      parent->setEmbededParameters(false);
    }
    else {
      for(int i=n-1; i>=0; i--) {
        auto *parameter = parent->getParameter(i);
        parameterBuffer.first = NULL;
        DOMNode *ps = parameter->getXMLElement()->getPreviousSibling();
        if(ps and X()%ps->getNodeName()=="#text")
          parameter->getXMLElement()->getParentNode()->removeChild(ps);
        parameter->getXMLElement()->getParentNode()->removeChild(parameter->getXMLElement());
        parent->removeParameter(parameter);
      }
    }
    parent->maybeRemoveEmbedXMLElement();
    model->removeRows(0,n,index);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadFrame(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML frame files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Frame *frame = Embed<Frame>::create(ele,parent);
    frame->setEmbeded(embed);
    frame->create();
    if(not frame) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      frame->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLFrames()->insertBefore(frame->getEmbedXMLElement(), nullptr);
      E(frame->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLFrames()->insertBefore(ele, nullptr);
    parent->addFrame(frame);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createFrameItem(frame,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadContour(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML contour files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Contour *contour = Embed<Contour>::create(ele,parent);
    contour->setEmbeded(embed);
    contour->create();
    if(not contour) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      contour->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLContours()->insertBefore(contour->getEmbedXMLElement(), nullptr);
      E(contour->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLContours()->insertBefore(ele, nullptr);
    parent->addContour(contour);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createContourItem(contour,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadGroup(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML group files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Group *group = Embed<Group>::create(ele,parent);
    group->setEmbeded(embed);
    group->create();
    if(not group) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      group->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLGroups()->insertBefore(group->getEmbedXMLElement(), nullptr);
      E(group->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLGroups()->insertBefore(ele, nullptr);
    parent->addGroup(group);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createGroupItem(group,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadObject(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML object files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Object *object = Embed<Object>::create(ele,parent);
    object->setEmbeded(embed);
    object->create();
    if(not object) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      object->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLObjects()->insertBefore(object->getEmbedXMLElement(), nullptr);
      E(object->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLObjects()->insertBefore(ele, nullptr);
    parent->addObject(object);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createObjectItem(object,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadLink(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML link files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Link *link = Embed<Link>::create(ele,parent);
    link->setEmbeded(embed);
    link->create();
    if(not link) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      link->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLLinks()->insertBefore(link->getEmbedXMLElement(), nullptr);
      E(link->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLLinks()->insertBefore(ele, nullptr);
    parent->addLink(link);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createLinkItem(link,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadConstraint(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML constraint files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Constraint *constraint = Embed<Constraint>::create(ele,parent);
    constraint->setEmbeded(embed);
    constraint->create();
    if(not constraint) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      constraint->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLConstraints()->insertBefore(constraint->getEmbedXMLElement(), nullptr);
      E(constraint->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLConstraints()->insertBefore(ele, nullptr);
    parent->addConstraint(constraint);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createConstraintItem(constraint,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadObserver(Element *parent, Element *element, bool embed) {
    setProjectChanged(true);
    DOMElement *ele = nullptr;
    QString file;
    auto *model = static_cast<ElementTreeModel*>(elementView->model());
    if(element) {
      ele = static_cast<DOMElement*>(doc->importNode(element->getEmbedXMLElement()?element->getEmbedXMLElement():element->getXMLElement(),true));
      if(elementBuffer.second) {
        elementBuffer.first = NULL;
        element->removeXMLElement();
        element->getParent()->removeElement(element);
        QModelIndex index = model->findItem(element,model->index(0,0));
        if(index.isValid())
          model->removeRow(index.row(), index.parent());
      }
    }
    else {
      file=QFileDialog::getOpenFileName(this, "XML observer files", ".", "XML files (*.xml)");
      if(not file.isEmpty()) {
        if(file.startsWith("//"))
          file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
        xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
        DOMParser::handleCDATA(doc->getDocumentElement());
        ele = embed?doc->getDocumentElement():static_cast<DOMElement*>(parent->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
      }
      else
        return;
    }
    Observer *observer = Embed<Observer>::create(ele,parent);
    observer->setEmbeded(embed);
    observer->create();
    if(not observer) {
      QMessageBox::warning(nullptr, "Import", "Cannot import file.");
      return;
    }
    if(embed) {
      observer->setEmbedXMLElement(D(doc)->createElement(PV%"Embed"));
      parent->getXMLObservers()->insertBefore(observer->getEmbedXMLElement(), nullptr);
      E(observer->getEmbedXMLElement())->setAttribute("href",getProjectDir().relativeFilePath(file).toStdString());
    }
    else
      parent->getXMLObservers()->insertBefore(ele, nullptr);
    parent->addObserver(observer);
    QModelIndex index = elementView->selectionModel()->currentIndex();
    model->createObserverItem(observer,index);
    QModelIndex currentIndex = index.child(model->rowCount(index)-1,0);
    elementView->selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::ClearAndSelect);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::loadSolver() {
    setProjectChanged(true);
    project->getSolver()->removeXMLElement();
    DOMElement *ele = NULL;
    QString file=QFileDialog::getOpenFileName(this, "XML frame files", ".", "XML files (*.xml)");
    if(not file.isEmpty()) {
      if(file.startsWith("//"))
        file.replace('/','\\'); // xerces-c is not able to parse files from network shares that begin with "//"
      xercesc::DOMDocument *doc = parser->parseURI(X()%file.toStdString());
      DOMParser::handleCDATA(doc->getDocumentElement());
      ele = static_cast<DOMElement*>(project->getXMLElement()->getOwnerDocument()->importNode(doc->getDocumentElement(),true));
    }
    else
      return;
    Solver *solver = Embed<Solver>::create(ele,project);
    solver->create();
    if(not solver) {
      QMessageBox::warning(0, "Import", "Cannot import file.");
      return;
    }
    project->getXMLElement()->insertBefore(ele, NULL);
    project->setSolver(solver);
    solverView->setSolver(solver);
    solverViewClicked();
  }

  void MainWindow::viewProjectSource() {
    SourceDialog dialog(getProject()->getXMLElement(),this);
    dialog.exec();
  }

  void MainWindow::viewElementSource() {
    QModelIndex index = elementView->selectionModel()->currentIndex();
    auto *element = dynamic_cast<Element*>(static_cast<ElementTreeModel*>(elementView->model())->getItem(index)->getItemData());
    if(element) {
      SourceDialog dialog(element->getXMLElement(),this);
      dialog.exec();
    }
  }

  void MainWindow::viewParametersSource() {
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *item = dynamic_cast<Parameters*>(static_cast<ElementTreeModel*>(parameterView->model())->getItem(index)->getItemData());
    if(item) {
      SourceDialog dialog(item->getItem()->getEmbedXMLElement()->getFirstElementChild(),this);
      dialog.exec();
    }
  }

  void MainWindow::viewSolverSource() {
    SourceDialog dialog(getProject()->getSolver()->getXMLElement(),this);
    dialog.exec();
  }

  void MainWindow::viewParameterSource() {
    QModelIndex index = parameterView->selectionModel()->currentIndex();
    auto *parameter = dynamic_cast<Parameter*>(static_cast<ParameterTreeModel*>(parameterView->model())->getItem(index)->getItemData());
    if(parameter) {
      SourceDialog dialog(parameter->getXMLElement(),this);
      dialog.exec();
    }
  }

  void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
      event->acceptProposedAction();
    }
  }

  void MainWindow::dropEvent(QDropEvent *event) {
    for (int i = 0; i < event->mimeData()->urls().size(); i++) {
      QString path = event->mimeData()->urls()[i].toLocalFile().toLocal8Bit().data();
      if(path.endsWith(".mbsx")) {
        QFile Fout(path);
        if (Fout.exists())
          loadProject(Fout.fileName());
      }
    }
  }

  void MainWindow::closeEvent(QCloseEvent *event) {
    if(maybeSave()) {
      QSettings settings;
      settings.setValue("mainwindow/geometry", saveGeometry());
      settings.setValue("mainwindow/state", saveState());
      settings.setValue("mainwindow/embeddingview/state", parameterView->header()->saveState());
      event->accept();
    }
    else
      event->ignore();
  }

  void MainWindow::showEvent(QShowEvent *event) {
    QSettings settings;
    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    restoreState(settings.value("mainwindow/state").toByteArray());
    elementView->header()->restoreState(settings.value("mainwindow/elementview/state").toByteArray());
    parameterView->header()->restoreState(settings.value("mainwindow/embeddingview/state").toByteArray());
    QMainWindow::showEvent(event);
  }

  void MainWindow::openRecentProjectFile() {
    if(maybeSave()) {
      auto *action = qobject_cast<QAction *>(sender());
      if (action)
        loadProject(action->data().toString());
    }
  }

  void MainWindow::setCurrentProjectFile(const QString &fileName) {

    QSettings settings;
    QStringList files = settings.value("mainwindow/recentProjectFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while(files.size() > maxRecentFiles)
      files.removeLast();

    settings.setValue("mainwindow/recentProjectFileList", files);

    foreach(QWidget *widget, QApplication::topLevelWidgets()) {
      auto *mainWin = dynamic_cast<MainWindow*>(widget);
      if(mainWin)
        mainWin->updateRecentProjectFileActions();
    }
  }

  void MainWindow::updateRecentProjectFileActions() {
    QSettings settings;
    QStringList files = settings.value("mainwindow/recentProjectFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)maxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
      QString text = QDir::current().relativeFilePath(files[i]);
      recentProjectFileActs[i]->setText(text);
      recentProjectFileActs[i]->setData(files[i]);
      recentProjectFileActs[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < maxRecentFiles; ++j)
      recentProjectFileActs[j]->setVisible(false);

    //separatorAct->setVisible(numRecentFiles > 0);
  }

  void MainWindow::settingsFinished(int result) {
    if(result != 0) {
      setProjectChanged(true);
      if(getAutoRefresh()) refresh();
    }
  }

  void MainWindow::applySettings() {
    setProjectChanged(true);
    if(getAutoRefresh()) refresh();
  }

  void MainWindow::interrupt() {
    echoView->clearOutput();
    echoView->addOutputText("<span class=\"MBSIMGUI_WARN\">Simulation interrupted</span>\n");
    echoView->updateOutput(true);
    process.terminate();
  }

  void MainWindow::kill() {
    echoView->clearOutput();
    echoView->addOutputText("<span class=\"MBSIMGUI_WARN\">Simulation killed</span>\n");
    echoView->updateOutput(true);
    process.kill();
  }

  void MainWindow::updateEchoView() {
    echoView->addOutputText(process.readAllStandardOutput().data());
    echoView->updateOutput(true);
  }

  void MainWindow::setAllowUndo(bool allowUndo_) {
    allowUndo = allowUndo_;
    actionUndo->setEnabled(allowUndo);
  }

  void MainWindow::updateStatus() {
    // call this function only every 0.25 sec
    if(statusTime.elapsed()<250)
      return;
    statusTime.restart();

    // show only last line
    string s=process.readAllStandardError().data();
    s.resize(s.length()-1);
    auto i=s.rfind('\n');
    i = i==string::npos ? 0 : i+1;
    statusBar()->showMessage(QString::fromStdString(s.substr(i)));
  }

  QString MainWindow::getProjectFilePath() const {
    return QUrl(QString::fromStdString(X()%doc->getDocumentURI())).toLocalFile();
  }

  void MainWindow::openProjectEditor() {
    if(not editorIsOpen()) {
      setAllowUndo(false);
      updateParameters(getProject());
      projectEditor = getProject()->createPropertyDialog();
      projectEditor->setAttribute(Qt::WA_DeleteOnClose);
      projectEditor->toWidget();
      projectEditor->show();
      connect(projectEditor,&ProjectPropertyDialog::finished,this,[=](){
        if(projectEditor->result()==QDialog::Accepted) {
          setProjectChanged(true);
          projectEditor->fromWidget();
          if(getAutoRefresh()) refresh();
          projectView->setText(project->getName());
        }
        setAllowUndo(true);
        projectEditor=nullptr;
      });
      connect(projectEditor,&ProjectPropertyDialog::apply,this,[=](){
        setProjectChanged(true);
        projectEditor->fromWidget();
        if(getAutoRefresh()) refresh();
        projectView->setText(project->getName());
      });
    }
  }

  void MainWindow::openElementEditor(bool config) {
    if(not editorIsOpen()) {
      setAllowUndo(false);
      QModelIndex index = elementView->selectionModel()->currentIndex();
      auto *element = dynamic_cast<Element*>(static_cast<ElementTreeModel*>(elementView->model())->getItem(index)->getItemData());
      if(element) {
        updateParameters(element);
        elementEditor = element->createPropertyDialog();
        elementEditor->setAttribute(Qt::WA_DeleteOnClose);
        if(config)
          elementEditor->toWidget();
        else
          elementEditor->setCancel(false);
        elementEditor->show();
        connect(elementEditor,&QDialog::finished,this,[=](){
          if(elementEditor->result()==QDialog::Accepted) {
            if(elementEditor->getCancel()) setProjectChanged(true);
            elementEditor->fromWidget();
            if(getAutoRefresh()) refresh();
          }
          setAllowUndo(true);
          elementEditor=nullptr;
        });
        connect(elementEditor,&ElementPropertyDialog::apply,this,[=](){
          if(elementEditor->getCancel()) setProjectChanged(true);
          elementEditor->fromWidget();
          if(getAutoRefresh()) refresh();
          elementEditor->setCancel(true);
        });
        connect(elementEditor,&ElementPropertyDialog::showXMLHelp,this,[=](){
          // generate url for current element
          string url="file://"+(installPath/"share"/"mbxmlutils"/"doc").string();
          string ns=element->getNameSpace().getNamespaceURI();
          replace(ns.begin(), ns.end(), ':', '_');
          replace(ns.begin(), ns.end(), '.', '_');
          replace(ns.begin(), ns.end(), '/', '_');
          url+="/"+ns+"/index.html#"+element->getType().toStdString();
          // open in XML help dialog
          xmlHelp(QString::fromStdString(url));
        });
      }
    }
  }

  void MainWindow::openParameterEditor(bool config) {
    if(not editorIsOpen()) {
      setAllowUndo(false);
      QModelIndex index = parameterView->selectionModel()->currentIndex();
      auto *parameter = dynamic_cast<Parameter*>(static_cast<ParameterTreeModel*>(parameterView->model())->getItem(index)->getItemData());
      if(parameter) {
        updateParameters(parameter->getParent(),true);
        parameterEditor = parameter->createPropertyDialog();
        parameterEditor->setAttribute(Qt::WA_DeleteOnClose);
        if(config)
          parameterEditor->toWidget();
        else
          parameterEditor->setCancel(false);
        parameterEditor->show();
        connect(parameterEditor,&QDialog::finished,this,[=](){
          if(parameterEditor->result()==QDialog::Accepted) {
            if(parameterEditor->getCancel()) setProjectChanged(true);
            parameterEditor->fromWidget();
            if(getAutoRefresh()) refresh();
            parameter->getParent()->updateStatus();
          }
        setAllowUndo(true);
        parameterEditor=nullptr;
        });
        connect(parameterEditor,&ParameterPropertyDialog::apply,this,[=](){
          if(parameterEditor->getCancel()) setProjectChanged(true);
          parameterEditor->fromWidget();
          if(getAutoRefresh()) refresh();
          parameterEditor->setCancel(true);
          parameter->getParent()->updateStatus();
        });
      }
    }
  }

  void MainWindow::openSolverEditor() {
    if(not editorIsOpen()) {
      setAllowUndo(false);
      updateParameters(getProject()->getSolver());
      solverEditor = getProject()->getSolver()->createPropertyDialog();
      solverEditor->setAttribute(Qt::WA_DeleteOnClose);
      solverEditor->toWidget();
      solverEditor->show();
      connect(solverEditor,&ProjectPropertyDialog::finished,this,[=](){
        if(solverEditor->result()==QDialog::Accepted) {
          setProjectChanged(true);
          solverEditor->fromWidget();
        }
        setAllowUndo(true);
        solverEditor=nullptr;
      });
      connect(solverEditor,&ProjectPropertyDialog::apply,this,[=](){
        setProjectChanged(true);
        solverEditor->fromWidget();
      });
    }
  }

}
