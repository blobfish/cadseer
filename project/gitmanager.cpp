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

#include <cassert>
#include <ostream>

#include <boost/variant.hpp>

#include <project/libgit2pp/src/index.hpp>
#include <project/libgit2pp/src/object.hpp>
#include <project/libgit2pp/src/commit.hpp>
#include <project/libgit2pp/src/branch.hpp>
#include <project/libgit2pp/src/ref.hpp>
#include <project/libgit2pp/src/signature.hpp>
#include <project/libgit2pp/src/exception.hpp>
#include <project/libgit2pp/src/revwalk.hpp>

#include <preferences/preferencesXML.h>
#include <preferences/manager.h>
#include <message/dispatch.h>
#include <message/observer.h>
#include <project/gitmanager.h>

using namespace prj;
using namespace git2;

static const std::string transName = "transaction";

//debug helpers
inline std::ostream& operator<<(std::ostream &stream, const Status &statusIn)
{
  stream <<
    ((statusIn.isCurrent()) ? "Is Current " : "") <<
    ((statusIn.isNewInIndex()) ? "Is New In Index " : "") <<
    ((statusIn.isModifiedInIndex()) ? "Is Modified In Index " : "") <<
    ((statusIn.isDeletedInIndex()) ? "Is Deleted In Index " : "") <<
    ((statusIn.isRenamedInIndex()) ? "Is Renamed In Index " : "") <<
    ((statusIn.isTypeChangedInIndex()) ? "Is Type Changed In Index " : "") <<
    ((statusIn.isNewInWorkdir()) ? "Is New In Work Directory " : "") <<
    ((statusIn.isModifiedInWorkdir()) ? "Is Modified In Work Directory " : "") <<
    ((statusIn.isDeletedInWorkdir()) ? "Is Deleted In Work Directory " : "") <<
    ((statusIn.isRenamedInWorkdir()) ? "Is Renamed In Work Directory " : "") <<
    ((statusIn.isTypeChangedInWorkdir()) ? "Is Type Changed In Work Directory" : "");
  
  return stream;
}

inline std::ostream& operator<<(std::ostream &streamIn, const StatusEntry &entryIn)
{
  //a status of 'NewInIndex' was causing a seg fault.
  //with a status of NewInIndex, _entry->index_to_workdir is null.
  streamIn <<
    ((!entryIn.oldPath().empty()) ? "Old Path is: " : "") << entryIn.oldPath() << std::endl <<
    ((!entryIn.newPath().empty()) ? "New Path is: " : "") << entryIn.newPath() << std::endl <<
    ((!entryIn.path().empty()) ? "Path is: " : "") << entryIn.path() << std::endl <<
    "Status: " << entryIn.status() << std::endl;
    
  return streamIn;
}

inline std::ostream& operator<<(std::ostream &streamIn, const StatusList &listIn)
{
  streamIn << "inside statuslist stream operator. entry count is: " << listIn.entryCount() << std::endl;
  for (std::size_t index = 0; index < listIn.entryCount(); ++index)
  {
    streamIn << std::endl << listIn.entryByIndex(index) << std::endl;
  }
  
  return streamIn; 
}

GitManager::GitManager()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "prj::GitManager";
  setupDispatcher();

}

GitManager::~GitManager()
{

}

void GitManager::create(const std::string& pathIn)
{
  git_repository_init_options opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
  opts.flags |= GIT_REPOSITORY_INIT_MKPATH;
  repo = Repository::init
  (
    pathIn,
    opts.flags,
    opts.mode,
    "",
    "CadSeer", //description
    "",
    "main", //initial head
    ""
  );
  
  assert(updateIndex());
  
  //create an empty first commit.
  SignatureBuilder sigBuilder(prf::manager().rootPtr->project().gitName(), prf::manager().rootPtr->project().gitEmail());
  Signature sig(sigBuilder);
  OId treeId = repo.index().writeTree();
  std::list<Commit> parents;
  
  repo.createCommit
  (
    "HEAD",
    sig,
    sig,
    "empty initial commit",
    repo.lookupTree(treeId),
    parents
  );
  
  createBranch(transName);
  std::string refName = "refs/heads/"; //seems a little hacky
  refName += transName;
  repo.setHead(refName);
}

void GitManager::open(const std::string& pathIn)
{
  repo = Repository::open(pathIn);
}

void GitManager::save()
{
  //something here to test and respond if index diff is NOT empty.
  
  git2::OId headCommit = repo.head().target();
  git2::Tree headTree = repo.lookupCommit(headCommit).tree();
  
  std::string masterName = "refs/heads/main";
  git2::Commit masterCommit = repo.lookupCommit(repo.lookupReference(masterName).target());
  git2::Tree masterTree = masterCommit.tree();
  
  //this is overkill. Shouldn't need to merge the trees as we could just use
  //the tree from HEAD. Leaving as an example of using git_merge_trees.
  git_index *rawIndex = repo.index().data();
  Exception::git2_assert
  (
    git_merge_trees
    (
      &rawIndex,
      repo.data(),
      masterTree.data(),
      masterTree.data(),
      headTree.data(),
      nullptr
    )
  );
  git2::OId outIndexTreeId = repo.index().writeTree();
  
  //build commit message.
  std::ostringstream stream;
  stream << "Save operation: see following" << std::endl << std::endl;
  git2::RevWalk walker = repo.createRevWalk();
  walker.pushHead();
  git2::OId nextId;
  while(walker.next(nextId) && nextId != masterCommit.oid())
  {
    stream << "    " << repo.lookupCommit(nextId).message() << std::endl;
  }
  
  SignatureBuilder sigBuilder(prf::manager().rootPtr->project().gitName(), prf::manager().rootPtr->project().gitEmail());
  Signature sig(sigBuilder);
  std::list<Commit> parents;
  parents.push_back(masterCommit);
  
  git2::OId newCommitId = repo.createCommit
  (
    "refs/heads/main",
    sig,
    sig,
    stream.str(),
    repo.lookupTree(outIndexTreeId),
    parents
  );
  
  //'squashed' commit should be created. Now move transaction branch to reference new commit.
  std::string refName = "refs/heads/"; //seems a little hacky
  refName += transName;
  git2::Reference trans = repo.lookupReference(refName);
  trans.setTarget(newCommitId);
}

bool GitManager::updateIndex()
{
   //check status.
  StatusList statusList = repo.listStatus
  (
    GIT_STATUS_SHOW_INDEX_AND_WORKDIR,
    GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_SORT_CASE_SENSITIVELY,
    std::vector<std::string>()
  );
  
  bool needCommit = false;
  for (std::size_t index = 0; index < statusList.entryCount(); ++index)
  {
    StatusEntry e = statusList.entryByIndex(index);
//     std::cout << e << std::endl;
    if
    (
      (e.status().isNewInWorkdir())
      || (e.status().isModifiedInWorkdir())
    )
    {
      repo.index().add(e.newPath());
      needCommit = true;
    }
    if (e.status().isDeletedInWorkdir())
    {
      repo.index().remove(e.oldPath());
      needCommit = true;
    }
  }
  if (!needCommit)
    return false;
  repo.index().write(); //doesn't seem to work without this.
  return true;
}

void GitManager::update()
{
  if (!updateIndex())
    return;
  
  //had a crash when putting sig builder constructor in sig constructor.
  SignatureBuilder sigBuilder(prf::manager().rootPtr->project().gitName(), prf::manager().rootPtr->project().gitEmail());
  Signature sig(sigBuilder);
  OId treeId = repo.index().writeTree();
  std::list<Commit> parents;
  parents.push_back(repo.lookupCommit(repo.lookupReferenceOId("HEAD")));
  
  if (commitMessage.empty())
    commitMessage = "no message";
  
  repo.createCommit
  (
    "HEAD",
    sig,
    sig,
    commitMessage,
    repo.lookupTree(treeId),
    parents
  );
  
  commitMessage.clear();
}

void GitManager::appendGitMessage(const std::string& message)
{
  if (gitMessagesFrozen)
    return;
  
  if (commitMessage.empty())
  {
    commitMessage = message;
  }
  else
  {
    //for some reason std::endl wasn't putting new line in commit message.
    //extra spaces make the merge --squash(file/save) message look good.
    std::ostringstream stream;
    stream << commitMessage << "\n    " << message;
    commitMessage = stream.str();
  }
}

void GitManager::createBranch(const std::string& nameIn)
{
  git2::Commit c = repo.lookupCommit(repo.lookupReferenceOId("HEAD"));
  git2::Branch b = repo.createBranch(nameIn, c, true);
}

git2::Commit GitManager::getCurrentHead()
{
  git2::Commit out;
  try
  {
    out = repo.lookupCommit(repo.head().target());
  }
  catch(const git2::Exception &e)
  {
    std::cerr << "ERROR: " << e.message() << ", in GitManager::getCurrentHead" << std::endl;
  }
  
  return out;
}

std::vector<git2::Commit> GitManager::getCommitsHeadToNamed(const std::string &referenceNameIn)
{
  git2::OId rid; //reference id.
  
  auto setIdOfPrefix = [&](const std::string &prefix)
  {
    std::ostringstream stream;
    stream << prefix << referenceNameIn;
    try {rid = repo.lookupReferenceOId(stream.str());}
    catch(const git2::Exception &) {}
  };
  
  std::vector<git2::Commit> out;
  
  if (!rid.isValid())
    setIdOfPrefix("refs/heads/");
  if (!rid.isValid())
    setIdOfPrefix("refs/tags/");
  if (!rid.isValid())
    return out;
  
  try
  {
    git2::Commit rCommit = repo.lookupCommit(rid);
    git2::RevWalk walker = repo.createRevWalk(); //why is createRevWalk not const?
    walker.pushHead();
    git2::OId nextId;
    while(walker.next(nextId) && nextId != rCommit.oid())
      out.push_back(repo.lookupCommit(nextId));
  }
  catch(const git2::Exception &)
  {
    out.clear();
  }
  
  return out;
}

void GitManager::resetHard(const std::string &commitIn)
{
  git2::OId cId;
  cId.fromString(commitIn);
  git2::Commit c = repo.lookupCommit(cId); 
  repo.reset(c, GIT_RESET_HARD);
}

std::vector<git2::Tag> GitManager::getTags()
{
  /* keep in mind that git has 2 types of tags: lightweight and annotate.
   * this was throwing me off as the lightweight tags have no object
   * in the git object database. We get an exception when lookupTag gets passed
   * a lightweights reference id, which is a commit. so we catch that and skip
   * as we are only interested in the annotation tags in this context.
   */
  std::vector<git2::Tag> out;
  
  std::list<std::string> tagStrings = repo.listTags();
  try
  {
    for (const auto &ts : tagStrings)
    {
      std::ostringstream stream;
      stream << "refs/tags/" << ts;
      try {out.push_back(repo.lookupTag(repo.lookupReferenceOId(stream.str())));}
      catch(const git2::Exception &e) {} //assuming lightweight tag.
    }
  }
  catch(const git2::Exception &e)
  {
    std::cerr << "ERROR: " << e.message() << ", in GitManager::getTags" << std::endl;
  }
  
  return out;
}

void GitManager::createTag(const std::string &name, const std::string &message)
{
  try
  {
    SignatureBuilder sigBuilder(prf::manager().rootPtr->project().gitName(), prf::manager().rootPtr->project().gitEmail());
    Signature sig(sigBuilder);
    
    repo.createTag
    (
      name,
      getCurrentHead(),
      sig,
      message,
      false //don't overwrite
    );
  }
  catch(const git2::Exception &e)
  {
    std::cerr << "ERROR: " << e.message() << ", in GitManager::createTag" << std::endl;
  }
}

void GitManager::destroyTag(const std::string &name)
{
  try
  {
    repo.deleteTag(name);
  }
  catch(const git2::Exception &e)
  {
    std::cerr << "ERROR: " << e.message() << ", in GitManager::destroyTag" << std::endl;
  }
}

void GitManager::checkoutTag(const git2::Tag &tag)
{
  try
  {
    git2::Commit tagCommit = repo.lookupCommit(tag.targetOid());
    git2::Branch newMain = repo.createBranch("main", tagCommit, true);
    //we can't overwrite the branch if head is pointing to it, so switch head temporarily.
    repo.setHead("refs/heads/main");
    git2::Branch newTransaction = repo.createBranch(transName, tagCommit, true);
    repo.setHead("refs/heads/transaction");
    repo.reset(tagCommit, GIT_RESET_HARD);
  }
  catch(const git2::Exception &e)
  {
    std::cerr << "ERROR: " << e.message() << ", in GitManager::checkOutTag" << std::endl;
  }
}

void GitManager::setupDispatcher()
{
  msg::Mask mask;
  
  mask = msg::Request | msg::Git | msg::Text;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&GitManager::gitMessageRequestDispatched, this, _1)));
  
  mask = msg::Request | msg::Git | msg::Freeze;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&GitManager::gitMessageFreezeDispatched, this, _1)));
  
  mask = msg::Request | msg::Git | msg::Thaw;
  observer->dispatcher.insert(std::make_pair(mask, boost::bind(&GitManager::gitMessageThawDispatched, this, _1)));
}

void GitManager::gitMessageRequestDispatched(const msg::Message &messageIn)
{
  Message pMessage = boost::get<prj::Message>(messageIn.payload);
  appendGitMessage(pMessage.gitMessage);
}

void GitManager::gitMessageFreezeDispatched(const msg::Message &)
{
  freezeGitMessages();
}

void GitManager::gitMessageThawDispatched(const msg::Message &)
{
  thawGitMessages();
}

GitMessageFreezer::GitMessageFreezer()
{
  observer = std::move(std::unique_ptr<msg::Observer>(new msg::Observer()));
  observer->name = "prj::GitMessageFreezer";
  freeze();
}

GitMessageFreezer::~GitMessageFreezer()
{
  thaw();
}

void GitMessageFreezer::freeze()
{
  observer->out(msg::Message(msg::Request | msg::Git | msg::Freeze));
}

void GitMessageFreezer::thaw()
{
  observer->out(msg::Message(msg::Request | msg::Git | msg::Thaw));
}
