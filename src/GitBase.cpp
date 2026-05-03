#include <GitBase.h>

#include <GitAsyncProcess.h>
#include <GitSyncProcess.h>

#include <QLogger>

using namespace QLogger;

#include <QDir>
#include <QFileInfo>

GitBase::GitBase(GitRepoConfig config)
   : mConfig(std::move(config))
{
   QFileInfo fileInfo(mConfig.gitDirectory);

   if (fileInfo.isFile())
   {
      QFile f(fileInfo.filePath());

      if (f.open(QIODevice::ReadOnly))
      {
         auto path = f.readAll().split(':').last().trimmed();
         mConfig.gitDirectory = mConfig.workingDirectory + "/" + path;
         f.close();
      }
   }
}

GitRepoConfig GitBase::config() const
{
   return mConfig;
}

QString GitBase::getWorkingDir() const
{
   return mConfig.workingDirectory;
}

void GitBase::setWorkingDir(const QString &workingDir)
{
   mConfig.workingDirectory = workingDir;
}

QString GitBase::getGitDir() const
{
   return mConfig.gitDirectory;
}

QString GitBase::getTopLevelRepo(const QString &path) const
{
   QLog_Trace("Git", "Updating the cached current branch");

   const auto cmd = QString("git -C \"%1\" rev-parse --show-toplevel").arg(path);

   QLog_Trace("Git", QString("Updating the cached current branch: {%1}").arg(cmd));

   const auto ret = run(cmd);

   return ret.success ? ret.output.trimmed() : QString {};
}

QString GitBase::gitLocation() const
{
   return mConfig.gitLocation;
}

GitExecResult GitBase::run(const QString &cmd) const
{
   GitSyncProcess p(mConfig);

   const auto ret = p.run(cmd);
   const auto runOutput = ret.output;

   if (ret.success && runOutput.contains("fatal:"))
      QLog_Info("Git", QString("Git command {%1} reported issues:\n%2").arg(cmd, runOutput));
   else if (!ret.success)
      QLog_Warning("Git", QString("Git command {%1} has errors:\n%2").arg(cmd, runOutput));

   return ret;
}

void GitBase::updateCurrentBranch()
{
   QLog_Trace("Git", "Updating the cached current branch");

   const auto cmd = QString("git rev-parse --abbrev-ref HEAD");

   QLog_Trace("Git", QString("Updating the cached current branch: {%1}").arg(cmd));

   const auto ret = run(cmd);

   mCurrentBranch = ret.success ? ret.output.trimmed().remove("heads/") : QString();
}

QString GitBase::getCurrentBranch()
{
   if (mCurrentBranch.isEmpty())
      updateCurrentBranch();

   return mCurrentBranch;
}

GitExecResult GitBase::getLastCommit() const
{
   QLog_Trace("Git", "Getting last commit");

   const auto cmd = QString("git rev-parse HEAD");

   QLog_Trace("Git", QString("Getting last commit: {%1}").arg(cmd));

   const auto ret = run(cmd);

   return ret;
}
