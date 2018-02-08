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

#include <boost/variant/variant_fwd.hpp>

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
    static const Mask Request(Mask().set(                        0));//!< message classification
    static const Mask Response(Mask().set(                       1));//!< message classification
    static const Mask Pre(Mask().set(                            2));//!< message response timing. think "about to". Data still valid
    static const Mask Post(Mask().set(                           3));//!< message response timing. think "done". Data invalid.
    static const Mask Preselection(Mask().set(                   4));//!< selection classification
    static const Mask Selection(Mask().set(                      5));//!< selection classification
    static const Mask Add(Mask().set(                            6));//!< modifier
    static const Mask Remove(Mask().set(                         7));//!< modifier
    static const Mask Clear(Mask().set(                          8));//!< modifier
    static const Mask SetCurrentLeaf(Mask().set(                 9));//!< project action
    static const Mask Feature(Mask().set(                       10));//!< project action & command
    static const Mask Update(Mask().set(                        11));//!< project action. request only for all: model, visual etc..
    static const Mask Force(Mask().set(                         12));//!< project action. request only. marks dirty to force updates
    static const Mask Model(Mask().set(                         13));//!< project action
    static const Mask Visual(Mask().set(                        14));//!< project action
    static const Mask Connection(Mask().set(                    15));//!< project action
    static const Mask Project(Mask().set(                       16));//!< application action
    static const Mask Dialog(Mask().set(                        17));//!< application action
    static const Mask Open(Mask().set(                          18));//!< application action
    static const Mask Close(Mask().set(                         19));//!< application action
    static const Mask Save(Mask().set(                          20));//!< application action
    static const Mask New(Mask().set(                           21));//!< application action
    static const Mask Git(Mask().set(                           22));//!< git project integration
    static const Mask Freeze(Mask().set(                        23));//!< modifier git message
    static const Mask Thaw(Mask().set(                          24));//!< modifier git message
    static const Mask Cancel(Mask().set(                        25));//!< command manager
    static const Mask Done(Mask().set(                          26));//!< command manager
    static const Mask Command(Mask().set(                       27));//!< command manager
    static const Mask Status(Mask().set(                        28));//!< display status
    static const Mask Text(Mask().set(                          29));//!< text
    static const Mask Overlay(Mask().set(                       30));//!< visual
    static const Mask Construct(Mask().set(                     31));//!< build.
    static const Mask Edit(Mask().set(                          32));//!< modify.
    static const Mask SetMask(Mask().set(                       33));//!< selection mask.
    static const Mask Info(Mask().set(                          34));//!< opens info window.
    static const Mask ThreeD(Mask().set(                        35));//!< visual
    static const Mask Show(Mask().set(                          36));//!< visual
    static const Mask Hide(Mask().set(                          37));//!< visual
    static const Mask Toggle(Mask().set(                        38));//!< visual
    static const Mask View(Mask().set(                          39));//!< visual
    static const Mask Top(Mask().set(                           40));//!< command
    static const Mask Front(Mask().set(                         41));//!< command
    static const Mask Right(Mask().set(                         42));//!< command
    static const Mask Iso(Mask().set(                           43));//!< command
    static const Mask HiddenLine(Mask().set(                    44));//!< command
    static const Mask Isolate(Mask().set(                       45));//!< command
    static const Mask Fit(Mask().set(                           46));//!< command
    static const Mask Fill(Mask().set(                          47));//!< command
    static const Mask Triangulation(Mask().set(                 48));//!< command
    static const Mask RenderStyle(Mask().set(                   49));//!< command
    static const Mask Box(Mask().set(                           50));//!< command
    static const Mask Sphere(Mask().set(                        51));//!< command
    static const Mask Cone(Mask().set(                          52));//!< command
    static const Mask Cylinder(Mask().set(                      53));//!< command
    static const Mask Blend(Mask().set(                         54));//!< command
    static const Mask Chamfer(Mask().set(                       55));//!< command
    static const Mask Draft(Mask().set(                         56));//!< command
    static const Mask Union(Mask().set(                         57));//!< command
    static const Mask Subtract(Mask().set(                      58));//!< command
    static const Mask Intersect(Mask().set(                     59));//!< command
    static const Mask Import(Mask().set(                        60));//!< command
    static const Mask Export(Mask().set(                        61));//!< command
    static const Mask OCC(Mask().set(                           62));//!< command
    static const Mask Step(Mask().set(                          63));//!< command
    static const Mask OSG(Mask().set(                           64));//!< command
    static const Mask Preferences(Mask().set(                   65));//!< command
    static const Mask SystemReset(Mask().set(                   66));//!< command
    static const Mask SystemToggle(Mask().set(                  67));//!< command
    static const Mask FeatureToSystem(Mask().set(               68));//!< command
    static const Mask SystemToFeature(Mask().set(               69));//!< command
    static const Mask DraggerToFeature(Mask().set(              70));//!< command
    static const Mask FeatureToDragger(Mask().set(              71));//!< command
    static const Mask DatumPlane(Mask().set(                    72));//!< command
    static const Mask CheckShapeIds(Mask().set(                 73));//!< command
    static const Mask DebugDump(Mask().set(                     74));//!< command
    static const Mask DebugShapeTrackUp(Mask().set(             75));//!< command
    static const Mask DebugShapeTrackDown(Mask().set(           76));//!< command
    static const Mask DebugShapeGraph(Mask().set(               77));//!< command
    static const Mask LinearMeasure(Mask().set(                 78));//!< command
    static const Mask CheckGeometry(Mask().set(                 79));//!< command
    static const Mask DebugInquiry(Mask().set(                  80));//!< command
    static const Mask Color(Mask().set(                         81));//!< command
    static const Mask Name(Mask().set(                          82));//!< command
    static const Mask DebugDumpProjectGraph(Mask().set(         83));//!< command
    static const Mask DebugDumpDAGViewGraph(Mask().set(         84));//!< command
    static const Mask Hollow(Mask().set(                        85));//!< command
    static const Mask Oblong(Mask().set(                        86));//!< command
    static const Mask Extract(Mask().set(                       87));//!< command
    static const Mask FeatureReposition(Mask().set(             88));//!< command
    static const Mask Squash(Mask().set(                        89));//!< command
    static const Mask Strip(Mask().set(                         90));//!< command
    static const Mask Nest(Mask().set(                          91));//!< command
    static const Mask DieSet(Mask().set(                        92));//!< command
    static const Mask Quote(Mask().set(                         93));//!< command
    static const Mask DAG(Mask().set(                           94));//!< descriptor move up
    static const Mask Dirty(Mask().set(                         95));//!< command
    static const Mask Refine(Mask().set(                        96));//!< command
    static const Mask Reorder(Mask().set(                       97));//!< project action. move up.
    static const Mask Current(Mask().set(                       98));//!< modifier referencing current coordinate system.
    static const Mask InstanceLinear(Mask().set(                99));//!< command, move.
    static const Mask InstanceMirror(Mask().set(               100));//!< command, move.
    static const Mask InstancePolar(Mask().set(                101));//!< command, move.
    static const Mask Skipped(Mask().set(                      102));//!< project action. move up.
    static const Mask SystemToSelection(Mask().set(            103));//!< command. move up
    static const Mask Offset(Mask().set(                       104));//!< command. move up
    static const Mask Thicken(Mask().set(                      105));//!< command. move up
    static const Mask Sew(Mask().set(                          106));//!< command. move up
  
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
      Message();
      Message(const Mask&);
      Message(const Payload&);
      Message(const Mask&, const Payload&);
      Mask mask;
      Payload payload;
    };

    typedef std::function< void (const Message&) > MessageHandler;
    typedef std::unordered_map<Mask, MessageHandler> MessageDispatcher;
    
    
    //@{
    //! Some convenient functions for common messages
    msg::Message buildGitMessage(const std::string &);
    msg::Message buildStatusMessage(const std::string &);
    msg::Message buildSelectionMask(slc::Mask);
    msg::Message buildShowThreeD(const boost::uuids::uuid&);
    msg::Message buildHideThreeD(const boost::uuids::uuid&);
    msg::Message buildShowOverlay(const boost::uuids::uuid&);
    msg::Message buildHideOverlay(const boost::uuids::uuid&);
    //@}
}



#endif // MSG_MESSAGE_H
