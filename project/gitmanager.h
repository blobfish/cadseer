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

#include <memory>

#include <project/libgit2pp/src/repository.hpp>
#include <project/libgit2pp/src/commit.hpp>
#include <project/libgit2pp/src/tag.hpp>

namespace msg{class Message; class Observer;}

namespace prj
{
  /*! @brief class for managin a git repository.
    * 
    * There are only 2 git branches allowed and they won't be exposed to the user.
    * Branches are 'main' and 'transaction'. The transaction branch will always be
    * ahead of the the main branch and the main branch will never get ahead of
    * the transaction branch. i.e. no fork. As the project updates, commits will be
    * added to the transaction branch. When the user 'saves' the file the transaction
    * branch will be merged and squashed into the main branch. 'Undo' will be limited
    * to the commits between the transaction branch and the main branch. 'Revisions'
    * are handled by git tags. Checking out of different revisions will only be allowed
    * when the transaction and main branches point to the same commit. When a checkout
    * of a revision happens both branches, main and transaction, will be moved to the
    * tag commit. This has potential for lost work when the current commit has not
    * been tagged.
    * 
    * @note only 2 branches, 'main' and 'transaction', that are not exposed to the user.
    * @note revision is synonymous for a git tag. User can add and remove tags. Loss of work.
    * @note potential loss of work when checking out a revision.
    */
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
    void freezeGitMessages(){gitMessagesFrozen = true;}
    void thawGitMessages(){gitMessagesFrozen = false;}
    bool areGitMessagesFrozen(){return gitMessagesFrozen;}
    
    git2::Commit getCurrentHead();
    
    /*! @brief get all commits from current head to given name.
    * 
    * @parameter referenceNameIn. just name, no directory prefixes
    * @return vector of commits
    * @note does not include commit of given name. will look in ref/heads and ref/tags
    */
    std::vector<git2::Commit> getCommitsHeadToNamed(const std::string &referenceNameIn);
    
    /*! @brief do a hard reset of the current head to given commit.
    * 
    * @parameter commitIn.
    */
    void resetHard(const std::string &commitIn);
    
    /*! @brief get all the tags that exist in the repo
    * 
    * @return vector of tags
    */
    std::vector<git2::Tag> getTags();
    
    /*! @brief create a tag/revision.
    * 
    * @parameter name of the revision to create
    * @parameter message of the revision to create.
    */
    void createTag(const std::string &name, const std::string &message);
    
    /*! @brief destroy tag/revision.
    * 
    * @parameter name of the revision to destroy
    */
    void destroyTag(const std::string &name);
    
    /*! @brief Switch to a revision.
    * 
    * @parameter tag to make current.
    * @note this will change the main and transaction branch.
    * Any 'tagless' commits will be lost.
    */
    void checkoutTag(const git2::Tag &tag);
    
    
    
  private:
    bool updateIndex(); //false means nothing has changed.
    void createBranch(const std::string &nameIn); //!< create a branch with name from current HEAD.
    git2::Repository repo;
    std::string commitMessage;
    bool gitMessagesFrozen = false;
    std::unique_ptr<msg::Observer> observer;
    void setupDispatcher();
    void gitMessageRequestDispatched(const msg::Message &);
    void gitMessageFreezeDispatched(const msg::Message &);
    void gitMessageThawDispatched(const msg::Message &);
  };
  
  class GitMessageFreezer
  {
  public:
    GitMessageFreezer();
    ~GitMessageFreezer();
    void freeze();
    void thaw();
  private:
    std::unique_ptr<msg::Observer> observer;
  };
}

#endif // PRJ_GITMANAGER_H
