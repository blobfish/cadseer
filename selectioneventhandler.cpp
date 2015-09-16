#include <iostream>
#include <assert.h>

#include <osg/Geometry>
#include <osg/View>
#include <osgViewer/View>
#include <osg/ValueObject>

#include "selectioneventhandler.h"
#include "nodemaskdefs.h"
#include "selectiondefs.h"
#include "selectionintersector.h"
#include "globalutilities.h"
#include "application.h"
#include "./project/project.h"
#include "./modelviz/connector.h"

using namespace osg;
using namespace boost::uuids;

std::ostream& operator<<(std::ostream& os, const SelectionContainer& container)
{
  os << 
    "type is: " << getNameOfType(container.selectionType) << 
    "      featureid is: " << GU::idToString(container.featureId) <<
    "      id is: " << GU::idToString(container.shapeId) << std::endl;
  return os;
}

std::ostream& operator<<(std::ostream& os, const SelectionContainers& containers)
{
  for (const auto &current : containers)
    os << current;
  return os;
}

SelectionEventHandler::SelectionEventHandler() : osgGA::GUIEventHandler()
{
    preHighlightColor = Vec4(1.0, 1.0, 0.0, 1.0);
    selectionColor = Vec4(1.0, 1.0, 1.0, 1.0);
    nodeMask = ~(NodeMaskDef::backGroundCamera | NodeMaskDef::gestureCamera | NodeMaskDef::csys);
}

void SelectionEventHandler::setSelectionMask(const unsigned int &maskIn)
{
    selectionMask = maskIn;

    if
    (
      ((SelectionMask::facesSelectable & selectionMask) == SelectionMask::facesSelectable) |
      ((SelectionMask::wiresSelectable & selectionMask) == SelectionMask::wiresSelectable) |
      ((SelectionMask::shellsSelectable & selectionMask) == SelectionMask::shellsSelectable) |
      ((SelectionMask::solidsSelectable & selectionMask) == SelectionMask::solidsSelectable) |
      ((SelectionMask::featuresSelectable & selectionMask) == SelectionMask::featuresSelectable) |
      ((SelectionMask::objectsSelectable & selectionMask) == SelectionMask::objectsSelectable)
    )
        nodeMask |= NodeMaskDef::face;
    else
        nodeMask &= ~NodeMaskDef::face;

    if
    (
      ((SelectionMask::edgesSelectable & selectionMask) == SelectionMask::edgesSelectable) |
      ((SelectionMask::wiresSelectable & selectionMask) == SelectionMask::wiresSelectable)
    )
        nodeMask |= NodeMaskDef::edge;
    else
        nodeMask &= ~NodeMaskDef::edge;

    if ((SelectionMask::verticesSelectable & selectionMask) == SelectionMask::verticesSelectable)
        nodeMask |= NodeMaskDef::vertex;
    else
        nodeMask &= ~NodeMaskDef::vertex;
}

bool SelectionEventHandler::handle(const osgGA::GUIEventAdapter& eventAdapter,
                    osgGA::GUIActionAdapter& actionAdapter, osg::Object *object,
                                   osg::NodeVisitor * nodeVistor)
{
    currentIntersections.clear();
    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::MOVE)
    {
        osg::View* view = actionAdapter.asView();
        if (!view)
            return false;

        osg::ref_ptr<SelectionIntersector>picker = new SelectionIntersector(
                    osgUtil::Intersector::WINDOW, eventAdapter.getX(),
                    eventAdapter.getY());
        picker->setPickRadius(16.0); //32 x 32 cursor

        osgUtil::IntersectionVisitor iv(picker.get());
        iv.setTraversalMask(nodeMask);
        view->getCamera()->accept(iv);
        if (picker->containsIntersections())
        {
           currentIntersections = picker->getIntersections();
           osgUtil::LineSegmentIntersector::Intersections::const_iterator itIt;

           SelectionContainer newContainer;
           for (itIt = currentIntersections.begin(); itIt != currentIntersections.end(); ++itIt)
           {
               SelectionContainer tempContainer;
               if (!buildPreSelection(tempContainer, itIt))
                   continue;

               if (alreadySelected(tempContainer))
                   continue;

               if (lastPrehighlight == tempContainer)
                   return false;

               newContainer = tempContainer;
               break;
           }

           if (newContainer.selections.size() < 1)
           {
               clearPrehighlight();
               return false;
           }

           //wires were screwing up edges when changing same edge belonging to 2 different wires in
           //the same function call. so we make sure the current prehighlight is empty.
           if (lastPrehighlight.selections.size() > 0)
           {
               clearPrehighlight();
               return false;
           }

           if (newContainer.selections.size() > 0)
               setPrehighlight(newContainer);
        }
        else
            clearPrehighlight();
    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::RELEASE)
    {
        if (lastPrehighlight.selections.size() > 0)
        {
            std::vector<Selected>::iterator it;
            for (it = lastPrehighlight.selections.begin(); it != lastPrehighlight.selections.end(); ++it)
            {
                Selected currentSelected = *it;
                assert(currentSelected.geometry.valid());
                setGeometryColor(currentSelected.geometry.get(), selectionColor);
            }
            selectionContainers.push_back(lastPrehighlight);
            lastPrehighlight = SelectionContainer();
        }
        else
            clearSelections();

    }

    if (eventAdapter.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON &&
            eventAdapter.getEventType() == osgGA::GUIEventAdapter::PUSH)
    {
        clearPrehighlight();
    }

    if (eventAdapter.getEventType() == osgGA::GUIEventAdapter::DRAG)
    {
        //don't get button info here, need to cache.
    }

    return false;
}

bool SelectionEventHandler::buildPreSelection(SelectionContainer &container,
                                              const osgUtil::LineSegmentIntersector::Intersections::const_iterator &intersection)
{
    osg::Geometry *geometry = dynamic_cast<Geometry *>((*intersection).drawable.get());
    if (!geometry)
        return false;

    assert(((*intersection).nodePath.size() - 4) >= 0);
    int localNodeMask = (*intersection).nodePath[(*intersection).nodePath.size() - 2]->getNodeMask();
    uuid selectedId = GU::getId((*intersection).nodePath.back());
    uuid featureId = GU::getId((*intersection).nodePath[(*intersection).nodePath.size() - 4]);

//    std::cout << std::endl << "buildPreselection: " << std::endl << "   localNodeMask is: " << localNodeMask << std::endl <<
//                 "   selectedId is: " << selectedId << std::endl <<
//                 "   featureId is: " << featureId << std::endl;

    container.featureId = featureId;
    switch (localNodeMask)
    {
    case NodeMaskDef::face:
        if ((selectionMask & SelectionMask::facesSelectable) == SelectionMask::facesSelectable)
        {
            Selected newSelection;
            newSelection.initialize(geometry);
            container.selections.push_back(newSelection);
            container.selectionType = SelectionTypes::Face;
            container.shapeId = selectedId;
        }
        else if ((selectionMask & SelectionMask::shellsSelectable) == SelectionMask::shellsSelectable)
        {
            const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const ModelViz::Connector &connector = feature->getConnector();
            std::vector<uuid> shells = connector.useGetParentsOfType(selectedId, TopAbs_SHELL);
            //should be only 1 shell
            if (shells.size() == 1)
            {
                std::vector<uuid> faces = connector.useGetChildrenOfType(shells.at(0), TopAbs_FACE);
                std::vector<osg::Geometry *> faceGeometry;
                getGeometryFromIds visit(faces, faceGeometry);
                (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
                std::vector<osg::Geometry *>::const_iterator geomIt;
                for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
                {
                    Selected newSelection;
                    newSelection.initialize(*geomIt);
                    container.selections.push_back(newSelection);
                }
                container.selectionType = SelectionTypes::Shell;
                container.shapeId = shells.at(0);
            }
        }
        else if ((selectionMask & SelectionMask::solidsSelectable) == SelectionMask::solidsSelectable)
        {
            const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const ModelViz::Connector &connector = feature->getConnector();
            std::vector<uuid> solids = connector.useGetParentsOfType(selectedId, TopAbs_SOLID);
            //should be only 1 solid
            if (solids.size() == 1)
            {
                std::vector<uuid> faces = connector.useGetChildrenOfType(solids.at(0), TopAbs_FACE);
                std::vector<osg::Geometry *> faceGeometry;
                getGeometryFromIds visit(faces, faceGeometry);
                (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
                std::vector<osg::Geometry *>::const_iterator geomIt;
                for (geomIt = faceGeometry.begin(); geomIt != faceGeometry.end(); ++geomIt)
                {
                    Selected newSelection;
                    newSelection.initialize(*geomIt);
                    container.selections.push_back(newSelection);
                }
                container.selectionType = SelectionTypes::Solid;
                container.shapeId = solids.at(0);
            }
        }
        break;
    case NodeMaskDef::edge:
        if ((selectionMask & SelectionMask::edgesSelectable) == SelectionMask::edgesSelectable)
        {
            Selected newSelection;
            newSelection.initialize(geometry);
            container.selectionType = SelectionTypes::Edge;
            container.shapeId = selectedId;
            container.selections.push_back(newSelection);
        }
        else if ((selectionMask & SelectionMask::wiresSelectable) == SelectionMask::wiresSelectable)
        {
            //try to find face.
            //for now assume the face is the next object in selection list.
            osgUtil::LineSegmentIntersector::Intersections::const_iterator faceIt = intersection;
            faceIt++;

            uuid faceId = boost::uuids::nil_generator()();
            while(faceIt != currentIntersections.end())
            {
                assert((*faceIt).nodePath.size() - 2 >= 0);
                int faceNodeMask = (*faceIt).nodePath[(*faceIt).nodePath.size() - 2]->getNodeMask();
                if (faceNodeMask == NodeMaskDef::face)
                {
                    faceId = GU::getId((*faceIt).nodePath.back());
                    break;
                }
                faceIt++;
            }
            if (faceId.is_nil())
                break;

            const Feature::Base *feature = dynamic_cast<Application *>(qApp)->getProject()->findFeature(featureId);
            assert(feature);
            const ModelViz::Connector &connector = feature->getConnector();
            uuid wireId = connector.useGetWire(selectedId, faceId);
            if (wireId == boost::uuids::nil_generator()())
                break;

            std::vector<uuid> edges = connector.useGetChildrenOfType(wireId, TopAbs_EDGE);
            std::vector<osg::Geometry *> edgeGeometry;
            getGeometryFromIds visit(edges, edgeGeometry);
            (*intersection).nodePath[(*intersection).nodePath.size() - 2]->accept(visit);
            std::vector<osg::Geometry *>::const_iterator geomIt;
            for (geomIt = edgeGeometry.begin(); geomIt != edgeGeometry.end(); ++geomIt)
            {
                Selected newSelection;
                newSelection.initialize(*geomIt);
                container.selections.push_back(newSelection);
            }
            container.selectionType = SelectionTypes::Wire;
            container.shapeId = wireId;
        }
        break;
    case NodeMaskDef::vertex:
    {
        Selected newSelection;
        newSelection.initialize(geometry);
        container.selectionType = SelectionTypes::Vertex;
        container.shapeId = selectedId;
        container.selections.push_back(newSelection);
        break;
    }
    default:
        assert(0);
        break;
    }

    return true;
}

void SelectionEventHandler::clearSelections()
{
    // clear in reverse order to fix wire issue were edges remained highlighted. edge was remembering already selected color.
    // something else will have to been when we get into delselection. maybe pool of selection and indexes for container?
    std::vector<SelectionContainer>::reverse_iterator it;
    for (it = selectionContainers.rbegin(); it != selectionContainers.rend(); ++it)
    {
        std::vector<Selected>::iterator selectedIt;
        for (selectedIt = it->selections.begin(); selectedIt != it->selections.end(); ++selectedIt)
        {
          if (!selectedIt->geometry.valid())
            continue;
          //should I lock the geometry observer point?
          setGeometryColor(selectedIt->geometry.get(), selectedIt->color);
        }
    }
    selectionContainers.clear();
}

bool SelectionEventHandler::alreadySelected(const SelectionContainer &testContainer)
{
    std::vector<SelectionContainer>::iterator it;
    for (it = selectionContainers.begin(); it != selectionContainers.end(); ++it)
    {
        if ((*it) == testContainer)
            return true;
    }
    return false;
}

void SelectionEventHandler::setPrehighlight(SelectionContainer &selected)
{
    lastPrehighlight = selected;
    std::vector<Selected>::const_iterator it;
    for (it = selected.selections.begin(); it != selected.selections.end(); ++it)
        setGeometryColor(it->geometry.get(), preHighlightColor);
}

void SelectionEventHandler::clearPrehighlight()
{
    if (lastPrehighlight.selections.size() < 1)
        return;
    std::vector<Selected>::const_iterator it;
    for (it = lastPrehighlight.selections.begin(); it != lastPrehighlight.selections.end(); ++it)
    {
      if (!it->geometry.valid())
        continue;
      //should I lock the geometry observer point?
      setGeometryColor(it->geometry.get(), it->color);
    }
    lastPrehighlight = SelectionContainer();
}

void SelectionEventHandler::setGeometryColor(osg::Geometry *geometryIn, const osg::Vec4 &colorIn)
{
    Vec4Array *colors = dynamic_cast<Vec4Array*>(geometryIn->getColorArray());
    assert(colors);
    assert(colors->size() > 0);

    (*colors)[0] = colorIn;
    colors->dirty();
    geometryIn->dirtyDisplayList();
}

void Selected::initialize(osg::Geometry *geometryIn)
{
    this->geometry = geometryIn;
    Vec4Array *colors = dynamic_cast<Vec4Array*>(geometryIn->getColorArray());
    assert(colors);
    assert(colors->size() > 0);
    this->color = (*colors)[0];
}

getGeometryFromIds::getGeometryFromIds(const std::vector<boost::uuids::uuid> &idsIn, std::vector<osg::Geometry *> &geometryIn) :
    osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), ids(idsIn), geometry(geometryIn)
{

}

void getGeometryFromIds::apply(osg::Geode &aGeode)
{
    std::string selectedIdString;
    if (!aGeode.getUserValue(GU::idAttributeTitle, selectedIdString))
        assert(0);
    uuid selectedId = GU::stringToId(selectedIdString);
    //consider switching connector and this visitor to a std::set instead of a vector for speed here.
    if (std::find(ids.begin(), ids.end(), selectedId) == ids.end())
    {
        traverse(aGeode);
        return;
    }
    osg::Geometry *geom = dynamic_cast<osg::Geometry *>(aGeode.getDrawable(0));
    assert(geom);
    geometry.push_back(geom);
}
