#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <vector>
#include <map>

#include <QObject>

#include <Standard_StdAllocator.hxx>
#include <TopoDS_Shape.hxx>

#include <osg/Switch>

//this is kind of goofy. Store all shapes in a vector that uses romans custom allocator.
//then use a map from hash to index into vector. this is to avoid writing a custom
//allocator for standard map.
typedef std::map<int, int> ShapeHashIndexMap; //first is hash of shape, second is vector index.
typedef std::vector<TopoDS_Shape, Standard_StdAllocator<TopoDS_Shape> > ShapeVector;
typedef std::map<int, osg::ref_ptr<osg::Switch> > ShapeHashSwitchMap;

class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(QObject *parent = 0);
    void readOCC(const std::string &fileName, osg::Group *root);
    TopoDS_Shape vizToShape(const osg::ref_ptr<osg::Switch> switchIn);
    osg::ref_ptr<osg::Switch> shapeToViz(const TopoDS_Shape &shape);
    
private:
    ShapeHashIndexMap map;
    ShapeVector shapeVector;
    ShapeHashSwitchMap switchMap;
};

#endif // DOCUMENT_H
