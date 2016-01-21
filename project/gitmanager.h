/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRJ_GITMANAGER_H
#define PRJ_GITMANAGER_H

#include <project/libgit2pp/src/repository.hpp>

namespace prj
{
  class GitManager
  {
  public:
    GitManager();
    ~GitManager();
    void open(const std::string &);
    void create(const std::string &);
    void update();
    void save();
    void appendGitMessage(const std::string &message);
    
  private:
    bool updateIndex(); //false means nothing has changed.
    void createBranch(const std::string &nameIn); //!< create a branch with name from current HEAD.
    git2::Repository repo;
    std::string commitMessage;
  };
}

#endif // PRJ_GITMANAGER_H
