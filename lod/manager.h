/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef LOD_MANAGER_H
#define LOD_MANAGER_H

#include <memory>

#include <boost/variant/variant.hpp>

#include <QObject>
#include <QProcess> //for enums.

#include <message/message.h>

class QTimer;
namespace msg{class Observer;}

namespace lod
{
//   enum class Status
//   {
//     Queued, //!< waiting.
//     Processing, //!< currently being processed.
//     Done //!< processing is done.
//   };
  
  struct Stow;
  
  /**
  * @brief Manage the external generation of lods
  * 
  * ref class owned by application.
  */
  class Manager : public QObject
  {
    Q_OBJECT
  public:
    Manager() = delete;
    Manager(const std::string&);
    virtual ~Manager() override;
  private:
    boost::filesystem::path lodPath; //!< path to external application
    std::vector<Message> messages; //message queue of all lods to be generated.
    Message cMessage; //!< current message being processed by child.
    bool cmValid = false; //!< current Message valid
    bool logging = false; //!< enable logging see constructor.
    bool childWorking = false;
    
    void send();
    void cleanMessages(const boost::uuids::uuid&);
    QProcess *process = nullptr;
    
    std::unique_ptr<msg::Observer> observer;
    void setupDispatcher();
    void constructLODRequestDispatched(const msg::Message &);
    void featureRemovedDispatched(const msg::Message &);
    void featureStateChangedDispatched(const msg::Message &);
    
  private Q_SLOTS:
    void readyReadStdOutSlot();
    void childFinishedSlot(int, QProcess::ExitStatus);
    void childErrorSlot(QProcess::ProcessError);
  };
}

#endif // LOD_MANAGER_H
