#include <GitAsyncProcess.h>

GitAsyncProcess::GitAsyncProcess(GitRepoConfig config)
   : AGitProcess(std::move(config))
{
}

GitExecResult GitAsyncProcess::run(const QString &command)
{
   const auto ret = execute(command);

   return { ret, "" };
}

void GitAsyncProcess::onFinished(int code, QProcess::ExitStatus exitStatus)
{
   AGitProcess::onFinished(code, exitStatus);

   if (!mCanceling)
      emit signalDataReady({ !mRealError, mRunOutput });

   deleteLater();
}
