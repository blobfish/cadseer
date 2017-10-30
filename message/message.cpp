/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/variant/variant.hpp>

#include <osg/Node>

#include <message/message.h>

using namespace msg;

Message::Message() : mask(0){}

Message::Message(const Mask &maskIn) : mask(maskIn){}

Message::Message(const Payload &payloadIn) : payload(payloadIn){}

Message::Message(const Mask &maskIn, const Payload &payloadIn) : mask(maskIn), payload(payloadIn){}

msg::Message msg::buildGitMessage(const std::string &messageIn)
{
  msg::Message out;
  out.mask = msg::Request | msg::Git | msg::Text;
  prj::Message pMessage;
  pMessage.gitMessage = messageIn;
  out.payload = pMessage;
  
  return out;
}

msg::Message msg::buildStatusMessage(const std::string &messageIn)
{
  msg::Message out;
  out.mask = msg::Request | msg::Status | msg::Text;
  vwr::Message statusMessage;
  statusMessage.text = messageIn;
  out.payload = statusMessage;
  
  return out;
}

msg::Message msg::buildSelectionMask(slc::Mask maskIn)
{
  slc::Message sMsg;
  sMsg.selectionMask = maskIn;
  msg::Message mMsg(msg::Request | msg::Selection | msg::SetMask);
  mMsg.payload = sMsg;
  
  return mMsg;
}

msg::Message msg::buildShowThreeD(const boost::uuids::uuid &idIn)
{
  vwr::Message vMsg;
  vMsg.featureId = idIn;
  msg::Message out(msg::Request | msg::View | msg::Show | msg::ThreeD);
  out.payload = vMsg;
  
  return out;
}

msg::Message msg::buildHideThreeD(const boost::uuids::uuid &idIn)
{
  vwr::Message vMsg;
  vMsg.featureId = idIn;
  msg::Message out(msg::Request | msg::View | msg::Hide | msg::ThreeD);
  out.payload = vMsg;
  
  return out;
}

msg::Message msg::buildShowOverlay(const boost::uuids::uuid &idIn)
{
  vwr::Message vMsg;
  vMsg.featureId = idIn;
  msg::Message out(msg::Request | msg::View | msg::Show | msg::Overlay);
  out.payload = vMsg;
  
  return out;
}

msg::Message msg::buildHideOverlay(const boost::uuids::uuid &idIn)
{
  vwr::Message vMsg;
  vMsg.featureId = idIn;
  msg::Message out(msg::Request | msg::View | msg::Hide | msg::Overlay);
  out.payload = vMsg;
  
  return out;
}
