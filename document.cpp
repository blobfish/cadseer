#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>

#include <osg/ValueObject>

#include "document.h"
#include "modelviz/graph.h"
#include "modelviz/connector.h"
#include "globalutilities.h"
#include "shapeobject.h"

Document::Document(QObject *parent) :
    QObject(parent)
{
}

void Document::readOCC(const std::string &fileName, osg::Group *root)
{
    TopoDS_Shape base;
    BRep_Builder junk;
    std::fstream file(fileName.c_str());
    BRepTools::Read(base, file, junk);

    //structure understood to be a compound of compounds.
    //check base is a compound.
    if (base.ShapeType() != TopAbs_COMPOUND)
    {
        std::cout << "expected base compound in reading of OCC" << std::endl;
        return;
    }

    TopoDS_Iterator it;
    for (it.Initialize(base); it.More(); it.Next())
    {
        TopoDS_Shape current = it.Value();
        if (current.ShapeType() != TopAbs_COMPOUND)
        {
            std::cout << "expected compound in reading of OCC. Got " << current.ShapeType() << std::endl;
//            continue;
        }

        ShapeObject *object = new ShapeObject(this);
        object->setShape(current);
        map.insert(std::make_pair(object->getShapeHash(), object));
        root->addChild(object->getMainSwitch());

//        ModelViz::BuildConnector connectBuilder(current);
//        connectBuilder.getConnector().outputGraphviz("graphTest");
    }
}
