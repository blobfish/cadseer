#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Iterator.hxx>

#include <osg/ValueObject>

#include "document.h"
#include "modelviz/graph.h"
#include "globalutilities.h"

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

    shapeVector.clear();
    map.clear();

    TopoDS_Iterator it;
    for (it.Initialize(base); it.More(); it.Next())
    {
        TopoDS_Shape current = it.Value();
        if (current.ShapeType() != TopAbs_COMPOUND)
        {
            std::cout << "expected compound in reading of OCC. Got " << current.ShapeType() << std::endl;
//            continue;
        }
        shapeVector.push_back(current);
        int hash = GU::getShapeHash(current);
        map.insert(std::make_pair(hash, shapeVector.size() - 1));

        ModelViz::Build builder(current);
        if (builder.go(0.25, 0.5))
        {
            root->addChild(builder.getViz().get());
            switchMap.insert(std::make_pair(hash, builder.getViz()));
        }
    //    builder.go(0.5, 1.0);
    //    builder.go(1.0, 2.0);
    }
}

TopoDS_Shape Document::vizToShape(const osg::ref_ptr<osg::Switch> switchIn)
{
    int hash;
    if (!switchIn->getUserValue("ShapeHash", hash))
    {
        std::cout << "no hash for shape" << std::endl;
        return TopoDS_Shape();
    }
    ShapeHashIndexMap::const_iterator it;
    it = map.find(hash);
    if (it == map.end())
    {
        std::cout << "could find hash in shape hash" << std::endl;
        return TopoDS_Shape();
    }
    return shapeVector.at(it->second);
}

osg::ref_ptr<osg::Switch> Document::shapeToViz(const TopoDS_Shape &shape)
{
    int hash;
    hash = GU::getShapeHash(shape);
    ShapeHashSwitchMap::const_iterator it;
    it = switchMap.find(hash);
    if (it == switchMap.end())
    {
        std::cout << "couldn't find shape in switch map" << std::endl;
        return osg::ref_ptr<osg::Switch>();
    }
    return it->second;
}
