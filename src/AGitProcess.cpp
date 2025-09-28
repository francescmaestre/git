#include "AGitProcess.h"

#include <QSettings>
#include <QTextStream>

#include <QLogger>

using namespace QLogger;

namespace
{
QString loginApp()
{
   const auto askPassApp = qEnvironmentVariable("SSH_ASKPASS");

   if (!askPassApp.isEmpty())
      return QString("%1=%2").arg(QString::fromUtf8("SSH_ASKPASS"), askPassApp);

#if defined(Q_OS_WIN)
   return QString("SSH_ASKPASS=win-ssh-askpass");
#else
   return QString("SSH_ASKPASS=ssh-askpass");
#endif
}

void restoreSpaces(QString &newCmd, const QChar &sepChar)
{
   QChar quoteChar;
   auto replace = false;
   const auto newCommandLength = newCmd.length();

   for (int i = 0; i < newCommandLength; ++i)
   {
      const auto c = newCmd[i];

      if (!replace && (c == "$"[0] || c == '\"' || c == '\'') && (newCmd.count(c) % 2 == 0))
      {
         replace = true;
         quoteChar = c;
         continue;
      }

      if (replace && (c == quoteChar))
      {
         replace = false;
         continue;
      }

      if (replace && c == sepChar)
         newCmd[i] = QChar(' ');
   }
}

QStringList splitArgList(const QString &cmd)
{
   // return argument list handling quotes and double quotes
   // substring, as example from:
   // cmd some_arg "some thing" v='some value'
   // to (comma separated fields)
   // sl = <cmd,some_arg,some thing,v='some value'>

   // early exit the common case
   if (!(cmd.contains("$") || cmd.contains("\"") || cmd.contains("\'")))
      return cmd.split(' ', Qt::SkipEmptyParts);

   // we have some work to do...
   // first find a possible separator
   const QString sepList("#%&!?"); // separator candidates
   int i = 0;
   while (cmd.contains(sepList[i]) && i < sepList.length())
      i++;

   if (i == sepList.length())
      return QStringList();

   const QChar &sepChar(sepList[i]);

   // remove all spaces
   QString newCmd(cmd);
   newCmd.replace(QChar(' '), sepChar);

   // re-add spaces in quoted sections
   restoreSpaces(newCmd, sepChar);

   // "$" is used internally to delimit arguments
   // with quoted text wholly inside as
   // arg1 = <[patch] cool patch on "cool feature">
   // and should be removed before to feed QProcess
   newCmd.remove("$");

   // QProcess::setArguments doesn't want quote
   // delimited arguments, so remove trailing quotes

   auto sl = QStringList(newCmd.split(sepChar, Qt::SkipEmptyParts));
   QStringList::iterator it(sl.begin());

   for (; it != sl.end(); ++it)
   {
      if (it->isEmpty())
         continue;

      if ((it->at(0) == QStringLiteral("\"") && it->right(1) == QStringLiteral("\""))
          || (it->at(0) == QStringLiteral("\'") && it->right(1) == QStringLiteral("\'")))
      {
         *it = it->mid(1, it->length() - 2);
      }
   }
   return sl;
}
}

QStringList AGitProcess::mExtraPaths {};

AGitProcess::AGitProcess(const QString &workingDir)
   : mWorkingDirectory(workingDir)
{
   qRegisterMetaType<GitExecResult>("GitExecResult");

   setWorkingDirectory(mWorkingDirectory);

   // Clone the current environment
   auto env = QProcessEnvironment::systemEnvironment();

   // Append /opt/homebrew/bin to PATH
   env.insert("PATH", env.value("PATH") + ":/opt/homebrew/bin");

   // Apply the modified environment to the process
   setProcessEnvironment(env);

   connect(this, &AGitProcess::readyReadStandardOutput, this, &AGitProcess::onReadyStandardOutput,
           Qt::DirectConnection);
   connect(this, static_cast<void (AGitProcess::*)(int, QProcess::ExitStatus)>(&AGitProcess::finished), this,
           &AGitProcess::onFinished, Qt::DirectConnection);
}

void AGitProcess::onCancel()
{

   mCanceling = true;

   waitForFinished();
}

void AGitProcess::setAdditionalPaths(const QStringList &paths)
{
   mExtraPaths = paths;
}

void AGitProcess::onReadyStandardOutput()
{
   if (!mCanceling)
   {
      const auto standardOutput = readAllStandardOutput();

      mRunOutput.append(QString::fromUtf8(standardOutput));

      emit procDataReady(standardOutput);
   }
}

bool AGitProcess::execute(const QString &command)
{
   mCommand = command;

   auto processStarted = false;
   auto arguments = splitArgList(mCommand);

   if (!arguments.isEmpty())
   {
      QStringList env = QProcess::systemEnvironment();
      env << "GIT_TRACE=0"; // avoid choking on debug traces
      env << "GIT_FLUSH=0"; // skip the fflush() in 'git log'
      env << loginApp();

      // Clone the current environment
      auto e = QProcessEnvironment::systemEnvironment();

      // Append /opt/homebrew/bin to PATH
      env << QString("PATH=%1:%2").arg(e.value("PATH"), mExtraPaths.join(":"));

      const auto gitAlternative = QSettings().value("gitLocation", "").toString();

      setEnvironment(env);
      setProgram(gitAlternative.isEmpty() ? arguments.takeFirst() : gitAlternative);
      setArguments(arguments);
      start();

      processStarted = waitForStarted();

      if (!processStarted)
         QLog_Warning("Git", QString("Unable to start the process:\n%1\nMore info:\n%2").arg(mCommand, errorString()));
      else
         QLog_Debug("Git", QString("Process started: %1").arg(mCommand));
   }

   return processStarted;
}

void AGitProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
   Q_UNUSED(exitCode)

   QLog_Debug("Git", QString("Process {%1} finished.").arg(mCommand));

   const auto errorOutput = readAllStandardError();

   mErrorOutput = QString::fromUtf8(errorOutput);
   mRealError = exitStatus != QProcess::NormalExit || mCanceling || mErrorOutput.contains("error", Qt::CaseInsensitive)
       || mErrorOutput.contains("fatal: ", Qt::CaseInsensitive)
       || mErrorOutput.contains("could not read username", Qt::CaseInsensitive);

   if (mRealError)
   {
      if (!mErrorOutput.isEmpty())
         mRunOutput = mErrorOutput;
   }
   else
      mRunOutput.append(readAllStandardOutput() + mErrorOutput);
}
