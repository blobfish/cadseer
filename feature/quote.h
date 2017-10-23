/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef FTR_QUOTE_H
#define FTR_QUOTE_H

#include <boost/filesystem/path.hpp>

#include <osg/ref_ptr>

#include <feature/base.h>

namespace lbr{class PLabel;}
namespace prj{namespace srl{class FeatureQuote;}}

namespace ftr
{
  struct QuoteData
  {
    int quoteNumber;
    QString customerName;
    boost::uuids::uuid customerId; //not used yet.
    QString partName;
    QString partNumber;
    QString partSetup;
    QString partRevision;
    QString materialType;
    double materialThickness; //eventually from somewhere else.
    QString processType;
    int annualVolume;
  };
  
  
  class Quote : public Base
  {
  public:
    constexpr static const char *strip = "Strip";
    constexpr static const char *dieSet = "DieSet";
    
    Quote();
    virtual ~Quote() override;
    
    virtual void updateModel(const UpdatePayload&) override;
    virtual Type getType() const override {return Type::Quote;}
    virtual const std::string& getTypeString() const override {return toString(Type::Quote);}
    virtual const QIcon& getIcon() const override {return icon;}
    virtual Descriptor getDescriptor() const override {return Descriptor::Create;}
    virtual void serialWrite(const QDir&) override;
    void serialRead(const prj::srl::FeatureQuote &);
    
    prm::Parameter tFile; //!< template file.
    prm::Parameter oFile; //!< output file.
    boost::filesystem::path pFile; //!< picture file.
    
    osg::ref_ptr<lbr::PLabel> tLabel;
    osg::ref_ptr<lbr::PLabel> oLabel;
    
    QuoteData quoteData;
    
  private:
    static QIcon icon;
  };
}

#endif // FTR_QUOTE_H
