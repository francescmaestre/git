#include <GitSyncProcess.h>

#include <QTemporaryFile>
#include <QTextStream>

GitSyncProcess::GitSyncProcess(GitRepoConfig config)
   : AGitProcess(std::move(config))
{
}

GitExecResult GitSyncProcess::run(const QString &command)
{
   const auto processStarted = execute(command);

   if (processStarted)
      waitForFinished(10000);

   close();

   return { !mRealError, mRunOutput };
}
