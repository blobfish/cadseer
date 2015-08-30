/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  tanderson <email>
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

#ifndef BLEND_H
#define BLEND_H

#include "base.h"

class BRepFilletAPI_MakeFillet;

namespace Feature
{
class Blend : public Base
{
  public:
    Blend();
    void setRadius(const double &radiusIn);
    double getRadius() const {return radius;}
    void setEdgeIds(const std::vector<boost::uuids::uuid>& edgeIdsIn);
    const std::vector<boost::uuids::uuid>& getEdgeIds(){return edgeIds;}
    
    virtual void update(const UpdateMap&) override;
    virtual Type getType() const override {return Type::Blend;}
    virtual const std::string& getTypeString() const override {return Feature::getTypeString(Type::Blend);}
  
  protected:
    double radius;
    std::vector<boost::uuids::uuid> edgeIds;
    
    /*! used to map the edges that are blended away to the face generated.
     * used to map new generated face to outer wire.
     */ 
    EvolutionContainer shapeMap;
    
    /*! map from known faces to new blended faces' edges and vertices.*/
    DerivedContainer derivedContainer;
    
private:
    void shapeMatch(const Feature::Base* targetFeatureIn);
    void modifiedMatch(BRepFilletAPI_MakeFillet&, const Base *);
    void generatedMatch(BRepFilletAPI_MakeFillet&, const Base *);
    void uniqueTypeMatch(const Base *);
    void outerWireMatch(const Base *);
    void derivedMatch(BRepFilletAPI_MakeFillet&, const Base *);
    
    void dumpInfo(BRepFilletAPI_MakeFillet&, const Base *);
    void dumpResultStats();
};
}

#endif // BLEND_H
