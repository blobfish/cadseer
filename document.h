#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <vector>
#include <map>

#include <QObject>

#include <Standard_StdAllocator.hxx>
#include <TopoDS_Shape.hxx>

#include <osg/Switch>

class ShapeObject;

//this is kind of goofy. Store all shapes in a vector that uses romans custom allocator.
//then use a map from hash to index into vector. this is to avoid writing a custom
//allocator for standard map.
typedef std::map<int, ShapeObject*> HashShapeObjectMap; //first is hash of shape, second is vector index.

class Document : public QObject
{
    Q_OBJECT
public:
    explicit Document(QObject *parent = 0);
    void readOCC(const std::string &fileName, osg::Group *root);
    ShapeObject* findShapeObjectFromHash(const int &hashIn);
    
private:
    HashShapeObjectMap map;
};

#endif // DOCUMENT_H
