/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef DLG_CHECKGEOMETRYDIALOG_H
#define DLG_CHECKGEOMETRYDIALOG_H

#include <memory>
#include <stack>
#include <set>

#include <boost/uuid/uuid.hpp>

#include <QDialog>

#include <TopAbs_ShapeEnum.hxx>

#include <osg/observer_ptr>

class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QTableWidget;
class QCloseEvent;
class QHideEvent;
class QTextEdit;

class BRepCheck_Analyzer;
class TopoDS_Shape;

namespace osg{class PositionAttitudeTransform;}

namespace ftr{class Base;}
namespace msg{class Message; class Observer;}

namespace dlg
{
  class BasicCheckPage : public QWidget
  {
    Q_OBJECT
  public:
    BasicCheckPage(const ftr::Base&, QWidget*);
    virtual ~BasicCheckPage() override;
    void go();
  protected:
    virtual void hideEvent(QHideEvent *) override;
  private:
    const ftr::Base &feature;
    std::unique_ptr<msg::Observer> observer;
    QTreeWidget *treeWidget = nullptr;
    std::stack<QTreeWidgetItem*> itemStack; //!< used during shape iteration.
    std::set<boost::uuids::uuid> checkedIds;
    osg::observer_ptr<osg::PositionAttitudeTransform> boundingSphere;
    
    void buildGui();
    void recursiveCheck(const BRepCheck_Analyzer &, const TopoDS_Shape &);
    void checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                                            TopAbs_ShapeEnum subType);
  private Q_SLOTS:
    void selectionChangedSlot();
  Q_SIGNALS:
    void basicCheckPassed();
    void basicCheckFailed();
  };
  
  class BOPCheckPage : public QWidget
  {
    Q_OBJECT
  public:
    BOPCheckPage(const ftr::Base&, QWidget*);
    virtual ~BOPCheckPage() override;
  public Q_SLOTS:
    void basicCheckFailedSlot();
    void basicCheckPassedSlot();
    void goSlot();
  private:
    const ftr::Base &feature;
    QTableWidget *tableWidget = nullptr;
    std::unique_ptr<msg::Observer> observer;
    osg::observer_ptr<osg::PositionAttitudeTransform> boundingSphere;
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ToleranceCheckPage : public QWidget
  {
    Q_OBJECT
  public:
    ToleranceCheckPage(const ftr::Base&, QWidget*);
    virtual ~ToleranceCheckPage() override;
    void go();
  protected:
    virtual void hideEvent(QHideEvent *) override;
  private:
    const ftr::Base &feature;
    std::unique_ptr<msg::Observer> observer;
    QTableWidget *tableWidget;
    osg::observer_ptr<osg::PositionAttitudeTransform> boundingSphere;
    void buildGui();
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ShapesPage : public QWidget
  {
    Q_OBJECT
  public:
    ShapesPage(const ftr::Base&, QWidget*);
    void go();
  private:
    const ftr::Base &feature;
    QTextEdit *textEdit;
    void buildGui();
  };
  
  class CheckGeometry : public QDialog
  {
    Q_OBJECT
  public:
    CheckGeometry(const ftr::Base&, QWidget*);
    virtual ~CheckGeometry() override;
    void go();
  protected:
    virtual void closeEvent (QCloseEvent*) override;
  private:
    const ftr::Base &feature;
    QTabWidget *tabWidget;
    BasicCheckPage *basicCheckPage;
    BOPCheckPage *bopCheckPage;
    ToleranceCheckPage *toleranceCheckPage;
    ShapesPage *shapesPage;
    std::unique_ptr<msg::Observer> observer;
    void buildGui();
  };
}

#endif // DLG_CHECKGEOMETRYDIALOG_H
