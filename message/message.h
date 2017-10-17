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

#ifndef MSG_MESSAGE_H
#define MSG_MESSAGE_H

#include <bitset>
#include <functional>
#include <unordered_map>

#include <boost/variant.hpp>

#include <project/message.h>
#include <selection/message.h>
#include <application/message.h>
#include <viewer/message.h>
#include <feature/message.h>

namespace msg
{
    //! Mask is for a key in a function dispatch message handler. We might need a validation routine.
    // no pre and post on requests only on response.
    typedef std::bitset<128> Mask;
    static const Mask Request(Mask().set(                        1));//!< message classification
    static const Mask Response(Mask().set(                       2));//!< message classification
    static const Mask Pre(Mask().set(                            3));//!< message response timing. think "about to". Data still valid
    static const Mask Post(Mask().set(                           4));//!< message response timing. think "done". Data invalid.
    static const Mask Preselection(Mask().set(                   5));//!< selection classification
    static const Mask Selection(Mask().set(                      6));//!< selection classification
    static const Mask Add(Mask().set(                            7));//!< modifier
    static const Mask Remove(Mask().set(                         8));//!< modifier
    static const Mask Clear(Mask().set(                          9));//!< modifier
    static const Mask SetCurrentLeaf(Mask().set(                10));//!< project action
    static const Mask Feature(Mask().set(                       11));//!< project action & command
    static const Mask Update(Mask().set(                        12));//!< project action. request only for all: model, visual etc..
    static const Mask ForceUpdate(Mask().set(                   13));//!< project action. request only. marks dirty to force updates
    static const Mask UpdateModel(Mask().set(                   14));//!< project action
    static const Mask UpdateVisual(Mask().set(                  15));//!< project action
    static const Mask Connection(Mask().set(                    16));//!< project action
    static const Mask OpenProject(Mask().set(                   17));//!< application action
    static const Mask CloseProject(Mask().set(                  18));//!< application action
    static const Mask SaveProject(Mask().set(                   19));//!< application action
    static const Mask NewProject(Mask().set(                    20));//!< application action
    static const Mask ProjectDialog(Mask().set(                 21));//!< application action
    static const Mask Git(Mask().set(                           22));//!< git project integration
    static const Mask Freeze(Mask().set(                        23));//!< modifier git message
    static const Mask Thaw(Mask().set(                          24));//!< modifier git message
    static const Mask Cancel(Mask().set(                        25));//!< command manager
    static const Mask Done(Mask().set(                          26));//!< command manager
    static const Mask Command(Mask().set(                       27));//!< command manager
    static const Mask Status(Mask().set(                        28));//!< display status
    static const Mask Text(Mask().set(                          29));//!< text
    static const Mask Overlay(Mask().set(                       30));//!< visual
    static const Mask Construct(Mask().set(                     31));//!< factory action.
    static const Mask ViewTop(Mask().set(                       32));//!< command
    static const Mask ViewFront(Mask().set(                     33));//!< command
    static const Mask ViewRight(Mask().set(                     34));//!< command
    static const Mask ViewFit(Mask().set(                       35));//!< command
    static const Mask ViewFill(Mask().set(                      36));//!< command
    static const Mask ViewTriangulation(Mask().set(             37));//!< command
    static const Mask Box(Mask().set(                           38));//!< command
    static const Mask Sphere(Mask().set(                        39));//!< command
    static const Mask Cone(Mask().set(                          40));//!< command
    static const Mask Cylinder(Mask().set(                      41));//!< command
    static const Mask Blend(Mask().set(                         42));//!< command
    static const Mask Chamfer(Mask().set(                       43));//!< command
    static const Mask Draft(Mask().set(                         44));//!< command
    static const Mask Union(Mask().set(                         45));//!< command
    static const Mask Subtract(Mask().set(                      46));//!< command
    static const Mask Intersect(Mask().set(                     47));//!< command
    static const Mask ImportOCC(Mask().set(                     48));//!< command
    static const Mask ExportOCC(Mask().set(                     49));//!< command
    static const Mask ExportOSG(Mask().set(                     50));//!< command
    static const Mask Preferences(Mask().set(                   51));//!< command
    static const Mask SystemReset(Mask().set(                   52));//!< command
    static const Mask SystemToggle(Mask().set(                  53));//!< command
    static const Mask FeatureToSystem(Mask().set(               54));//!< command
    static const Mask SystemToFeature(Mask().set(               55));//!< command
    static const Mask DraggerToFeature(Mask().set(              56));//!< command
    static const Mask FeatureToDragger(Mask().set(              57));//!< command
    static const Mask DatumPlane(Mask().set(                    58));//!< command
    static const Mask CheckShapeIds(Mask().set(                 59));//!< command
    static const Mask DebugDump(Mask().set(                     60));//!< command
    static const Mask DebugShapeTrackUp(Mask().set(             61));//!< command
    static const Mask DebugShapeTrackDown(Mask().set(           62));//!< command
    static const Mask ViewInfo(Mask().set(                      63));//!< command
    static const Mask DebugShapeGraph(Mask().set(               64));//!< command
    static const Mask LinearMeasure(Mask().set(                 65));//!< command
    static const Mask CheckGeometry(Mask().set(                 66));//!< command
    static const Mask SetMask(Mask().set(                       67));//!< selection mask. move up someday.
    static const Mask ViewIsolate(Mask().set(                   68));//!< command. move up someday.
    static const Mask Info(Mask().set(                          69));//!< window move up someday.
    static const Mask DebugInquiry(Mask().set(                  70));//!< command move up someday.
    static const Mask Color(Mask().set(                         71));//!< command move up someday.
    static const Mask Edit(Mask().set(                          72));//!< modifier move up someday.
    static const Mask Name(Mask().set(                          73));//!< modifier move up someday.
    static const Mask DebugDumpProjectGraph(Mask().set(         74));//!< modifier move up someday.
    static const Mask DebugDumpDAGViewGraph(Mask().set(         75));//!< modifier move up someday.
    static const Mask Hollow(Mask().set(                        76));//!< command move up someday.
    static const Mask ViewToggleHiddenLine(Mask().set(          77));//!< command move up someday.
    static const Mask ExportStep(Mask().set(                    78));//!< command move up someday.
    static const Mask ViewIso(Mask().set(                       79));//!< command move up someday.
    static const Mask Oblong(Mask().set(                        80));//!< command move up someday.
    static const Mask ThreeD(Mask().set(                        81));//!< visual
    static const Mask Show(Mask().set(                          82));//!< visual
    static const Mask Hide(Mask().set(                          83));//!< visual
    static const Mask Toggle(Mask().set(                        84));//!< visual
    static const Mask Extract(Mask().set(                       85));//!< command move up someday.
    static const Mask ImportStep(Mask().set(                    86));//!< command move up someday.
    static const Mask FeatureReposition(Mask().set(             87));//!< command move up someday.
    static const Mask Squash(Mask().set(                        88));//!< command move up someday.
    static const Mask Strip(Mask().set(                         89));//!< command move up someday.
    static const Mask Nest(Mask().set(                          90));//!< command move up someday.
    static const Mask DieSet(Mask().set(                        91));//!< command move up someday.
    static const Mask Quote(Mask().set(                         92));//!< command move up someday.
    static const Mask View(Mask().set(                          93));//!< command move up someday.
    static const Mask RenderStyle(Mask().set(                   94));//!< command move up someday.
  
    typedef boost::variant
    <
      prj::Message,
      slc::Message,
      app::Message,
      vwr::Message,
      ftr::Message
    > Payload;
  
    struct Message
    {
    Message(){}
    Message(const Mask &maskIn) : mask(maskIn){}
    Mask mask = 0;
    Payload payload;
    };

    typedef std::function< void (const Message&) > MessageHandler;
    typedef std::unordered_map<Mask, MessageHandler> MessageDispatcher;
    
    
    //@{
    //! Some convenient functions for common messages
    msg::Message buildGitMessage(const std::string &);
    msg::Message buildStatusMessage(const std::string &);
    msg::Message buildSelectionMask(slc::Mask);
    //@}
}



#endif // MSG_MESSAGE_H
