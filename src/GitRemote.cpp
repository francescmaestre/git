#include "GitRemote.h"

#include <GitBase.h>
#include <GitConfig.h>
#include <GitSubmodules.h>

#include <QLogger>

using namespace QLogger;

GitRemote::GitRemote(const QSharedPointer<GitBase> &gitBase)
   : mGitBase(gitBase)
{
}

GitExecResult GitRemote::pushBranch(const QString &branchName, bool force)
{
   QLog_Debug("Git", QString("Executing push"));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   const auto ret = gitConfig->getRemoteForBranch(branchName);
   const auto remote = ret.success && !ret.output.isEmpty() ? ret.output : QString("origin");

   return mGitBase->run(QString("git push %1 %2 %3").arg(remote, branchName, force ? QString("--force") : QString()));
}

GitExecResult GitRemote::push(bool force)
{
   QLog_Debug("Git", QString("Executing push"));

   const auto ret = mGitBase->run(QString("git push ").append(force ? QString("--force") : QString()));

   return ret;
}

GitExecResult GitRemote::pushCommit(const QString &sha, const QString &remoteBranch)
{
   QLog_Debug("Git", QString("Executing pushCommit"));

   QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
   const auto remote = gitConfig->getRemoteForBranch(remoteBranch);

   return mGitBase->run(QString("git push %1 %2:refs/heads/%3")
                            .arg(remote.success ? remote.output : QString("origin"), sha, remoteBranch));
}

GitExecResult GitRemote::pull(bool updateSubmodulesOnPull)
{
   QLog_Debug("Git", QString("Executing pull"));

   auto ret = mGitBase->run("git pull");

   if (ret.success && updateSubmodulesOnPull)
   {
      QScopedPointer<GitSubmodules> git(new GitSubmodules(mGitBase));
      const auto updateRet = git->submoduleUpdate(QString());

      if (!updateRet)
      {
         return { updateRet,
                  "There was a problem updating the submodules after pull. Please review that you don't have any local "
                  "modifications in the submodules" };
      }
   }

   return ret;
}

bool GitRemote::fetch(bool autoPrune)
{
   QLog_Debug("Git", QString("Executing fetch with prune"));

   const auto cmd
       = QString("git fetch --all --tags --force %1").arg(autoPrune ? QString("--prune --prune-tags") : QString());
   const auto ret = mGitBase->run(cmd).success;

   return ret;
}

bool GitRemote::fetchBranch(const QString &branch) const
{
   QLog_Debug("Git", QString("Executing fetch over a branch"));

   QString remoteName;

   {
      QScopedPointer<GitConfig> gitConfig(new GitConfig(mGitBase));
      auto ret = gitConfig->getRemoteForBranch(branch);
      if (ret.success)
         remoteName = ret.output;
   }

   const auto cmd = QString("git fetch %1 %2").arg(remoteName, branch);
   const auto ret = mGitBase->run(cmd).success;

   return ret;
}

GitExecResult GitRemote::prune()
{
   QLog_Debug("Git", QString("Executing prune"));

   const auto ret = mGitBase->run("git remote prune origin");

   return ret;
}

GitExecResult GitRemote::addRemote(const QString &remoteRepo, const QString &remoteName)
{
   QLog_Debug("Git", QString("Adding a remote repository"));

   const auto ret = mGitBase->run(QString("git remote add %1 %2").arg(remoteName, remoteRepo));

   if (ret.success)
   {
      const auto ret2 = mGitBase->run(QString("git fetch %1").arg(remoteName));
   }

   return ret;
}

GitExecResult GitRemote::removeRemote(const QString &remoteName)
{
   QLog_Debug("Git", QString("Removing a remote repository"));

   return mGitBase->run(QString("git remote rm %1").arg(remoteName));
}

GitExecResult GitRemote::getRemotes() const
{
   QLog_Debug("Git", QString("Getting the list of all remote repositories"));

   return mGitBase->run("git remote");
}
