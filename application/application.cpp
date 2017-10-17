/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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
 */

#include <iostream>
#include <memory>

#include <QTimer>
#include <QMessageBox>
#include <QSettings>

#include <project/libgit2pp/libgit2/include/git2/global.h> //for git start and shutdown.

#include <application/application.h>
#include <application/mainwindow.h>
#include <viewer/spaceballqevent.h>
#include <viewer/viewerwidget.h>
#include <project/gitmanager.h> //needed for unique_ptr destructor call.
#include <project/project.h>
#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <message/dispatch.h>
#include <message/message.h>
#include <message/observer.h>
#include <application/factory.h>
#include <command/manager.h>
#include <project/projectdialog.h>

#include <spnav.h>


using namespace app;

Application::Application(int &argc, char **argv) :
    QApplication(argc, argv)
{
    qRegisterMetaType<msg::Message>("msg::Message");
  
    spaceballPresent = false;
    observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
    observer->name = "app::Application";
    setupDispatcher();
    
    //didn't have any luck using std::make_shared. weird behavior.
    std::unique_ptr<MainWindow> tempWindow(new MainWindow());
    mainWindow = std::move(tempWindow);
    
    std::unique_ptr<Factory> tempFactory(new Factory());
    factory = std::move(tempFactory);
    
    cmd::manager(); //just to construct the singleton and get it ready for messages.
    
    //some day pass the preferences file from user home directory to the manager constructor.
    prf::Manager &manager = prf::manager(); //this should cause preferences file to be read.
    if (!manager.isOk())
    {
      QMessageBox::critical(0, tr("CadSeer"), tr("Preferences failed to load"));
      QTimer::singleShot(0, this, SLOT(quit()));
      return;
    }
    
    setOrganizationName("blobfish");
    setOrganizationDomain("blobfish.org"); //doesn't exist.
    setApplicationName("cadseer");
    
    git_libgit2_init();
}

Application::~Application()
{
  git_libgit2_shutdown();
  spnav_close();
}

void Application::appStartSlot()
{
  //a project might be loaded before we launch the message queue.
  //if so then don't do the dialog.
  if (!project)
  {
    prj::Dialog dialog(this->getMainWindow());
    if (dialog.exec() == QDialog::Accepted)
    {
      prj::Dialog::Result r = dialog.getResult();
      if (r == prj::Dialog::Result::Open || r == prj::Dialog::Result::Recent)
      {
        openProject(dialog.getDirectory().absolutePath().toStdString());
      }
      if (r == prj::Dialog::Result::New)
      {
        createNewProject(dialog.getDirectory().absolutePath().toStdString());
      }
    }
    else
    {
      this->quit();
    }
  }
  
  getMainWindow()->getViewer()->getGraphicsWidget()->setFocus();
}

void Application::quittingSlot()
{
    if (!spnav_close())// the not seems goofy.
    {
//        std::cout << "spaceball disconnected" << std::endl;
    }
    else
    {
//        std::cout << "couldn't disconnect spaceball" << std::endl;
    }
}

void Application::messageSlot(msg::Message messageIn)
{
  //can't block, message might be open file or something we need to handle here.
  observer->out(messageIn);
}

bool Application::notify(QObject* receiver, QEvent* e)
{
  try
  {
    bool outDefault = QApplication::notify(receiver, e);
    if (e->type() == spb::MotionEvent::Type)
    {
      spb::MotionEvent *motionEvent = dynamic_cast<spb::MotionEvent*>(e);
      if
      (
        (!motionEvent) ||
        (motionEvent->isHandled())
      )
        return true;
        
      //make a new event and post to parent.
      spb::MotionEvent *newEvent = new spb::MotionEvent(*motionEvent);
      QObject *theParent = receiver->parent();
      if (!theParent || theParent == mainWindow.get())
        postEvent(mainWindow->getViewer()->getGraphicsWidget(), newEvent);
      else
        postEvent(theParent, newEvent);
      return true;
    }
    else if(e->type() == spb::ButtonEvent::Type)
    {
      spb::ButtonEvent *buttonEvent = dynamic_cast<spb::ButtonEvent*>(e);
      if
      (
        (!buttonEvent) ||
        (buttonEvent->isHandled())
      )
        return true;
        
      //make a new event and post to parent.
      spb::ButtonEvent *newEvent = new spb::ButtonEvent(*buttonEvent);
      QObject *theParent = receiver->parent();
      if (!theParent || theParent == mainWindow.get())
        postEvent(mainWindow->getViewer()->getGraphicsWidget(), newEvent);
      else
        postEvent(theParent, newEvent);
      return true;
    }
    else
      return outDefault;
  }
  catch(...)
  {
    std::cerr << "unhandled exception in Application::notify" << std::endl;
  }
  return false;
}

void Application::initializeSpaceball()
{
    if (!mainWindow)
        return;

    spb::registerEvents();

    if (spnav_open() == -1)
    {
      std::cout << "No spaceball found" << std::endl;
    }
    else
    {
      std::cout << "Spaceball found" << std::endl;
      spaceballPresent = true;
      QTimer *spaceballTimer = new QTimer(this);
      spaceballTimer->setInterval(1000/30); //30 times a second
      connect(spaceballTimer, &QTimer::timeout, this, &Application::spaceballPollSlot);
      spaceballTimer->start();
    }
}

void Application::spaceballPollSlot()
{
  spnav_event navEvent;
  if (!spnav_poll_event(&navEvent))
    return;
  
  QWidget *currentWidget = qApp->focusWidget();
  if (!currentWidget)
    return;

  if (navEvent.type == SPNAV_EVENT_MOTION)
  {
    spb::MotionEvent *qEvent = new spb::MotionEvent();
    qEvent->setTranslations(navEvent.motion.x, navEvent.motion.y, navEvent.motion.z);
    qEvent->setRotations(navEvent.motion.rx, navEvent.motion.ry, navEvent.motion.rz);
    this->postEvent(currentWidget, qEvent);
    return;
  }

  if (navEvent.type == SPNAV_EVENT_BUTTON)
  {
    spb::ButtonEvent *qEvent = new spb::ButtonEvent();
    qEvent->setButtonNumber(navEvent.button.bnum);
    if (navEvent.button.press == 1)
      qEvent->setButtonStatus(spb::BUTTON_PRESSED);
    else
      qEvent->setButtonStatus(spb::BUTTON_RELEASED);
    this->postEvent(currentWidget, qEvent);
    return;
  }
}

QDir Application::getApplicationDirectory()
{
  //if windows wants hidden, somebody else will have to do it.
  QString appDirName = ".CadSeer";
  QString homeDirName = QDir::homePath();
  QDir appDir(homeDirName + QDir::separator() + appDirName);
  if (!appDir.exists())
    appDir.mkpath(homeDirName + QDir::separator() + appDirName);
  return appDir;
}

QSettings& Application::getUserSettings()
{
  static QSettings *out = nullptr;
  if (!out)
  {
    QString fileName = getApplicationDirectory().absolutePath() + QDir::separator() + "QSettings.ini";
    out = new QSettings(fileName, QSettings::IniFormat, this);
  }
  return *out;
}

void Application::createNewProject(const std::string &directoryIn)
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::NewProject;
  observer->out(preMessage);
  
  //directoryIn has been verified to exist before this call.
  std::unique_ptr<prj::Project> tempProject(new prj::Project());
  project = std::move(tempProject);
  project->setSaveDirectory(directoryIn);
  project->initializeNew();
  
  updateTitle();
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::NewProject;
  observer->out(postMessage);
}

void Application::openProject(const std::string &directoryIn)
{
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::OpenProject;
  observer->out(preMessage);
  
  assert(!project);
  //directoryIn has been verified to exist before this call.
  std::unique_ptr<prj::Project> tempProject(new prj::Project());
  project = std::move(tempProject);
  project->setSaveDirectory(directoryIn);
  project->open();
  
  updateTitle();
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::OpenProject;
  observer->out(postMessage);
  
  msg::Message viewFitMessage;
  viewFitMessage.mask = msg::Request | msg::ViewFit;
  observer->out(viewFitMessage);
}

void Application::closeProject()
{
  //something here for modified project.
  
  msg::Message preMessage;
  preMessage.mask = msg::Response | msg::Pre | msg::CloseProject;
  observer->out(preMessage);
 
  if (project)
  {
    //test modified. qmessagebox to save. save if applicable.
    project.reset();
  }
  
  msg::Message postMessage;
  postMessage.mask = msg::Response | msg::Post | msg::CloseProject;
  observer->out(postMessage);
}

void Application::updateTitle()
{
  if (!project)
    return;
  QDir directory(QString::fromStdString(project->getSaveDirectory()));
  QString name = directory.dirName();
  QString title = tr("CadSeer --") + name + "--";
  mainWindow->setWindowTitle(title);
}

void Application::setupDispatcher()
{
  msg::Mask mask;
  mask = msg::Request | msg::NewProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Application::newProjectRequestDispatched, this, _1)));
  mask = msg::Request | msg::OpenProject;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Application::openProjectRequestDispatched, this, _1)));
  mask = msg::Request | msg::ProjectDialog;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&Application::ProjectDialogRequestDispatched, this, _1)));
}

void Application::ProjectDialogRequestDispatched(const msg::Message&)
{
  prj::Dialog dialog(this->getMainWindow());
  if (dialog.exec() == QDialog::Accepted)
  {
    prj::Dialog::Result r = dialog.getResult();
    if (r == prj::Dialog::Result::Open || r == prj::Dialog::Result::Recent)
    {
      closeProject();
      openProject(dialog.getDirectory().absolutePath().toStdString());
    }
    if (r == prj::Dialog::Result::New)
    {
      closeProject();
      createNewProject(dialog.getDirectory().absolutePath().toStdString());
    }
  }
}

void Application::newProjectRequestDispatched(const msg::Message &messageIn)
{
  std::ostringstream debug;
  debug << "inside: " << __PRETTY_FUNCTION__ << std::endl;
  msg::dispatch().dumpString(debug.str());
  
  prj::Message pMessage = boost::get<prj::Message>(messageIn.payload);
  
  QDir dir(QString::fromStdString(pMessage.directory));
  if (!dir.mkpath(QString::fromStdString(pMessage.directory)))
  {
    QMessageBox::critical(this->getMainWindow(), tr("Failed building directory"), QString::fromStdString(pMessage.directory));
    return;
  }
  
  if(project)
    closeProject();
  
  createNewProject(pMessage.directory);
}

void Application::openProjectRequestDispatched(const msg::Message &messageIn)
{
  prj::Message pMessage = boost::get<prj::Message>(messageIn.payload);
  
  QDir dir(QString::fromStdString(pMessage.directory));
  if (!dir.exists())
  {
    QMessageBox::critical(this->getMainWindow(), tr("Failed finding directory"), QString::fromStdString(pMessage.directory));
    return;
  }
  
  if(project)
    closeProject();
  
  openProject(pMessage.directory);
}

WaitCursor::WaitCursor()
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
}

WaitCursor::~WaitCursor()
{
  QApplication::restoreOverrideCursor();
}
