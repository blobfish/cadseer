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

#ifndef Q_MOC_RUN
#include <boost/uuid/uuid.hpp>
#endif

#include <QDialog>

#include <TopAbs_ShapeEnum.hxx>

#include <osg/observer_ptr>
#include <osg/BoundingSphere>

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
namespace ann{class SeerShape;}

namespace dlg
{
  class CheckPageBase : public QWidget
  {
    Q_OBJECT
  public:
    CheckPageBase(const ftr::Base&, QWidget*);
    virtual ~CheckPageBase() override;
  protected:
    std::unique_ptr<msg::Observer> observer;
    const ftr::Base &feature;
    const ann::SeerShape &seerShape;
    osg::observer_ptr<osg::PositionAttitudeTransform> boundingSphere;
    osg::BoundingSphered minBoundingSphere;
  };
  
  class BasicCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    BasicCheckPage(const ftr::Base&, QWidget*);
    virtual ~BasicCheckPage() override;
    void go();
  protected:
    virtual void hideEvent(QHideEvent *) override;
  private:
    QTreeWidget *treeWidget = nullptr;
    std::stack<QTreeWidgetItem*> itemStack; //!< used during shape iteration.
    std::set<boost::uuids::uuid> checkedIds;
    
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
  
  class BOPCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    BOPCheckPage(const ftr::Base&, QWidget*);
    virtual ~BOPCheckPage() override;
  public Q_SLOTS:
    void basicCheckFailedSlot();
    void basicCheckPassedSlot();
    void goSlot();
  protected:
    virtual void hideEvent(QHideEvent *) override;
  private:
    QTableWidget *tableWidget = nullptr;
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ToleranceCheckPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    ToleranceCheckPage(const ftr::Base&, QWidget*);
    virtual ~ToleranceCheckPage() override;
    void go();
  protected:
    virtual void hideEvent(QHideEvent *) override;
  private:
    QTableWidget *tableWidget;
    void buildGui();
    
  private Q_SLOTS:
    void selectionChangedSlot();
  };
  
  class ShapesPage : public CheckPageBase
  {
    Q_OBJECT
  public:
    ShapesPage(const ftr::Base&, QWidget*);
    void go();
  private:
    QTextEdit *textEdit;
    QPushButton *boundary;
    void buildGui();
    void selectBoundary();
  protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void hideEvent(QHideEvent *) override;
  private Q_SLOTS:
    void goBoundarySlot(bool checked);
  };
  
  /*! @brief dialog for displaying corrupt occt geometry.
   * 
   * class assumes that the feature passed in does have
   * a valid SeerShape.
   */
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
    void setupDispatcher();
    void buildGui();
    void featureRemovedDispatched(const msg::Message &);
    void featureStateChangedDispatched(const msg::Message &);
  };
}

#endif // DLG_CHECKGEOMETRYDIALOG_H
