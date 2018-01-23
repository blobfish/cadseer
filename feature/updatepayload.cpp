



#include <feature/base.h>
#include <feature/updatepayload.h>


using namespace ftr;

std::vector<const Base*> UpdatePayload::getFeatures(const std::string &tag) const
{
  std::vector<const Base*> out;
  for (auto its = updateMap.equal_range(tag); its.first != its.second; ++its.first)
    out.push_back(its.first->second);
  
  return out;
}

std::vector<const Base*> UpdatePayload::getFeatures(const UpdateMap &updateMapIn, const std::string &tag)
{
  std::vector<const Base*> out;
  for (auto its = updateMapIn.equal_range(tag); its.first != its.second; ++its.first)
    out.push_back(its.first->second);
  
  return out;
}
